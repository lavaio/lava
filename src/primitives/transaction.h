// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_H
#define BITCOIN_PRIMITIVES_TRANSACTION_H

#include <stdint.h>
#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <ticket.h>
#include <primitives/confidential.h>
#include <primitives/txwitness.h>

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

// CA:
// Globals to avoid circular dependencies.
extern bool g_con_elementsmode;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    //
    // CA flags:

    /* If this flag is set, the CTxIn including this COutPoint has a
     * CAssetIssuance object. */
    static const uint32_t OUTPOINT_ISSUANCE_FLAG = (1 << 31);

    /* The inverse of the combination of the preceding flags. Used to
     * extract the original meaning of `n` as the index into the
     * transaction's output array. */
    static const uint32_t OUTPOINT_INDEX_MASK = 0x3fffffff;

    // END CA
    //

    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    COutPoint(): n(NULL_INDEX) { }
    COutPoint(const uint256& hashIn, uint32_t nIn): hash(hashIn), n(nIn) { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = NULL_INDEX; }
    bool IsNull() const { return (hash.IsNull() && n == NULL_INDEX); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScriptWitness scriptWitness; //!< Only serialized through CTransaction              
    std::vector<unsigned char> vchIssuanceAmountRangeproof;
    std::vector<unsigned char> vchInflationKeysRangeproof;
    CAssetIssuance assetIssuance;

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1U << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {

        //
        // CA:

        bool fHasAssetIssuance;
        COutPoint outpoint;
        if (!ser_action.ForRead()) {
            if (prevout.n == (uint32_t) -1) {
                // Coinbase inputs do not have asset issuances attached
                // to them.
                fHasAssetIssuance = false;
                outpoint = prevout;
            } else {
                // The issuance and pegin bits can't be set as it is used to indicate
                // the presence of the asset issuance or pegin objects. They should
                // never be set anyway as that would require a parent
                // transaction with over one billion outputs.
                assert(!(prevout.n & ~COutPoint::OUTPOINT_INDEX_MASK));
                // The assetIssuance object is used to represent both new
                // asset generation and reissuance of existing asset types.
                fHasAssetIssuance = !assetIssuance.IsNull();
                // The mode is placed in the upper bits of the outpoint's
                // index field. The IssuanceMode enum values are chosen to
                // make this as simple as a bitwise-OR.
                outpoint.hash = prevout.hash;
                outpoint.n = prevout.n & COutPoint::OUTPOINT_INDEX_MASK;
                if (fHasAssetIssuance) {
                    outpoint.n |= COutPoint::OUTPOINT_ISSUANCE_FLAG;
                }
            }
        }

        READWRITE(outpoint);

        if (ser_action.ForRead()) {
            if (outpoint.n == (uint32_t) -1) {
                // No asset issuance for Coinbase inputs.
                fHasAssetIssuance = false;
                prevout = outpoint;
            } else {
                // The presence of the asset issuance object is indicated by
                // a bit set in the outpoint index field.
                fHasAssetIssuance = !!(outpoint.n & COutPoint::OUTPOINT_ISSUANCE_FLAG);
                // The mode, if set, must be masked out of the outpoint so
                // that the in-memory index field retains its traditional
                // meaning of identifying the index into the output array
                // of the previous transaction.
                prevout.hash = outpoint.hash;
                prevout.n = outpoint.n & COutPoint::OUTPOINT_INDEX_MASK;
            }
        }

        // END CA
        //

        READWRITE(scriptSig);
        READWRITE(nSequence);

        // CA:
        // The asset fields are deserialized only if they are present.
        if (fHasAssetIssuance) {
            READWRITE(assetIssuance);
            if (assetIssuance.IsNull()) {
                throw std::ios_base::failure("Superfluous issuance record");
            }
        } else if (ser_action.ForRead()) {
            assetIssuance.SetNull();
        }
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence     == b.nSequence &&
                a.assetIssuance == b.assetIssuance);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    CConfidentialAsset nAsset;
    CConfidentialValue nValueCA;
    CConfidentialNonce nNonce;
    std::vector<unsigned char> vchSurjectionproof;
    std::vector<unsigned char> vchRangeproof;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn,const CConfidentialAsset& nAssetIn, const CConfidentialValue& nValueCAIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (g_con_elementsmode) {
            READWRITE(nAsset);
            READWRITE(nValue);
            READWRITE(nNonce);
            READWRITE(scriptPubKey);
        } else {
            CAmount value;
            if (!ser_action.ForRead()) {
                value = nValue.GetAmount();
            }
            READWRITE(value);
            if (ser_action.ForRead()) {
                nValue.SetToAmount(value);
            }
            READWRITE(scriptPubKey);
        }
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
        nAsset.SetNull();
        nValueCA.SetNull();
        nNonce.SetNull();
    }

    bool IsNull() const
    {
        if (!g_con_elementsmode) {
            // Ignore the asset and the nonce in compatibility mode.
            return nValueCA.IsNull() && scriptPubKey.empty();
        }

        return (nValue == -1)&&nAsset.IsNull() && nValueCA.IsNull() && nNonce.IsNull() && scriptPubKey.empty();
    }

    bool IsFee() const {
        return g_con_elementsmode && scriptPubKey == CScript()
            && nValueCA.IsExplicit() && nAsset.IsExplicit();
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nAsset == b.nAsset &&
                a.nValue == b.nValue &&
                a.nNonce == b.nNonce &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {

    s >> tx.nVersion;
    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    tx.witness.SetNull();

    // Witness serialization is different between CA and Core.
    // See code comments in SerializeTransaction for details about the differences.
    if (g_con_elementsmode) {
        s >> flags;
        s >> tx.vin;
        s >> tx.vout;
        s >> tx.nLockTime;
        if (flags & 1) {
            /* The witness flag is present. */
            flags ^= 1;
            const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
            const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
            s >> tx.witness;
            if (!tx.HasWitness()) {
                /* It's illegal to encode witnesses when all witness stacks are empty. */
                throw std::ios_base::failure("Superfluous witness record");
            }
        }
    } else {
        const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

        /* Try to read the vin. In case the dummy is there, this will be read as an empty vector. */
        s >> tx.vin;
        if (tx.vin.size() == 0 && fAllowWitness) {
            /* We read a dummy or an empty vin. */
            s >> flags;
            if (flags != 0) {
                s >> tx.vin;
                s >> tx.vout;
            }
        } else {
            /* We read a non-empty vin. Assume a normal vout follows. */
            s >> tx.vout;
        }

        if ((flags & 1) && fAllowWitness) {
            /* The witness flag is present. */
            flags ^= 1;
            const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
            const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
            for (size_t i = 0; i < tx.vin.size(); i++) {
                s >> tx.witness.vtxinwit[i].scriptWitness.stack;
            }
        }
        s >> tx.nLockTime;
    }

    if (flags) {
        /* Unknown flag in the serialization */
        throw std::ios_base::failure("Unknown transaction optional data");
    }
}

template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

    s << tx.nVersion;

    // Consistency check
    assert(tx.witness.vtxinwit.size() <= tx.vin.size());
    assert(tx.witness.vtxoutwit.size() <= tx.vout.size());

    // Check whether witnesses need to be serialized.
    unsigned char flags = 0;
    if (fAllowWitness && tx.HasWitness()) {
        flags |= 1;
    }

    // Witness serialization is different between CA and Core.
    if (g_con_elementsmode) {
        // In CA-style serialization, all normal data is serialized first and the
        // witnesses all in the end.
        s << flags;
        s << tx.vin;
        s << tx.vout;
        s << tx.nLockTime;
        if (flags & 1) {
            const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
            const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
            s << tx.witness;
        }
    } else {
        // In Core-style serialization, we encode the input dummy and the witnesses
        // follow the outputs before the nLockTime.

        if (flags) {
            /* Use extended format in case witnesses are to be serialized. */
            std::vector<CTxIn> vinDummy;
            s << vinDummy;
            s << flags;
        }
        s << tx.vin;
        s << tx.vout;
        if (flags & 1) {
            const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
            const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
            for (size_t i = 0; i < tx.vin.size(); i++) {
                s << tx.witness.vtxinwit[i].scriptWitness.stack;
            }
        }
        s << tx.nLockTime;
    }
}


/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;

    // Confidential transaction version.
    static const int32_t CONFIDENTIAL_VERSION=3;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=3;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const int32_t nVersion;
    const uint32_t nLockTime;

private:
    /** Memory only. */
    const uint256 hash;
    const uint256 m_witness_hash;

    uint256 ComputeHash() const;
    uint256 ComputeWitnessHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    explicit CTransaction(const CMutableTransaction &tx);
    CTransaction(CMutableTransaction &&tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const { return hash; }
    const uint256& GetWitnessHash() const { return m_witness_hash; };
    // CA: the witness only hash used in CA witness roots
    uint256 GetWitnessOnlyHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }

    bool IsTicketTx() const;

    CTicketRef Ticket() const;
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    int32_t nVersion;
    uint32_t nLockTime;

    CMutableTransaction();
    explicit CMutableTransaction(const CTransaction& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }

    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

#endif // BITCOIN_PRIMITIVES_TRANSACTION_H
