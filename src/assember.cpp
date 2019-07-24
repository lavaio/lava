#include <assember.h>
#include <chainparams.h>
#include <logging.h>
#include <miner.h>
#include <poc.h>
#include <util/time.h>
#include <validation.h>
#include <key_io.h>
#include <actiondb.h>

#include <boost/bind.hpp>

CPOCBlockAssember::CPOCBlockAssember()
    : scheduler(std::make_shared<CScheduler>())
{
    SetNull();
}

bool CPOCBlockAssember::UpdateDeadline(const int height, const CKeyID& keyid, const uint64_t nonce, const uint64_t deadline)
{
    auto prevIndex = chainActive.Tip();
    
    if (prevIndex->nHeight != (height - 1)) {
        LogPrintf("chainActive has been update, the new index is %uul, but the height to be produced is %uul\n", prevIndex->nHeight, height);
		return false;
	}
    auto params = Params();
    if (deadline / prevIndex->nBaseTarget > params.TargetDeadline()) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    if (this->deadline != 0 && deadline >= this->deadline) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    auto plotID = keyid.GetPlotID();
    auto generationSignature = CalcGenerationSignature(prevIndex->genSign, prevIndex->nPlotID);
    if (CalcDeadline(generationSignature, height, plotID, nonce) != deadline) {
        LogPrintf("Deadline inconformity %uul\n", deadline);
        return false;
    }
    auto ts = (deadline / prevIndex->nBaseTarget);
    LogPrintf("Update new deadline: %u, now: %u, target: %u\n", ts, GetTimeMillis() / 1000, prevIndex->nTime + ts);

    {
        boost::lock_guard<boost::mutex> lock(mtx);
        this->height = height;
        this->keyid = keyid;
        this->genSig = genSig;
        this->nonce = nonce;
        this->deadline = deadline;
        auto lastBlockTime = prevIndex->GetBlockHeader().GetBlockTime();
        auto ts = ((deadline / chainActive.Tip()->nBaseTarget) * 1000);
        this->dl = (lastBlockTime * 1000) + ts;
    }
    return true;
}

void CPOCBlockAssember::CreateNewBlock()
{
    int height{ 0 };
    CKeyID form;
    uint256 genSig;
    uint64_t deadline{ 0 };
    uint64_t nonce{ 0 };
    {
        boost::lock_guard<boost::mutex> lock(mtx);
        height = this->height;
        form = this->keyid;
        genSig = this->genSig;
        nonce = this->nonce;
        deadline = this->deadline;
    }
    auto plotid = form.GetPlotID();
    LogPrintf("CPOCBlockAssember CreateNewBlock, plotid: %u nonce:%u newheight:%u deadline:%u utc:%u\n", plotid, nonce, height, deadline, GetTimeMillis()/1000);
    auto params = Params();
    auto to = g_relationdb->To(form);
    auto target = to.IsNull() ? form : to;
    auto scriptPubKeyIn = GetScriptForDestination(CTxDestination(target));
    auto blk = BlockAssembler(params).CreateNewBlock(scriptPubKeyIn, nonce, plotid, deadline);
    if (blk) {
        uint32_t extraNonce = 0;
        IncrementExtraNonce(&blk->block, chainActive.Tip(), extraNonce);
        auto pblk = std::make_shared<CBlock>(blk->block);
        if (ProcessNewBlock(params, pblk, true, NULL) == false) {
            LogPrintf("ProcessNewBlock failed\n");
        }
    } else {
        LogPrintf("CreateNewBlock failed\n");
    }
}

void CPOCBlockAssember::CheckDeadline()
{
    if (dl == 0)
        return;
    if (GetTimeMillis() >= dl) {
        CreateNewBlock();
        SetNull();
    }
}

void CPOCBlockAssember::SetNull()
{
    height = 0;
    nonce = 0;
    deadline = 0;
    genSig = uint256();
    keyid.SetNull();
    dl = 0;
}
