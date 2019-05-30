#include "assember.h"
#include "poc.h"
#include "chainparams.h"
#include "logging.h"
#include "miner.h"
#include "util/time.h"
#include "validation.h"

CPOCBlockAssember::CPOCBlockAssember()
    : plotID(0), nonce(0), deadline(-1)
{
    genSig.SetNull();
}

bool CPOCBlockAssember::UpdateDeadline(const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script)
{
    if (deadline >= this->deadline) {
        //TODO... log
        return false;
    }
    auto block = chainActive.Tip()->GetBlockHeader();
    auto generationSignature = CalcGenerationSignature(block.genSign, block.nPlotID);
    if (CalcDeadline(generationSignature, chainActive.Tip()->nHeight + 1, plotID, nonce) != deadline) {
        //TODO... log
        return false;
    }
    auto params = Params();
    if (deadline / chainActive.Tip()->nBaseTarget > params.GetTargetDeadline()) {
        return false;
    }
    
    //TODO.. stop old timer
    this->genSig = genSig;
    this->plotID = plotID;
    this->deadline = deadline;
    this->nonce = nonce;
    auto lastBlockTime = chainActive.Tip()->GetBlockHeader().GetBlockTime();
    //auto dl = lastBlockTime + ((deadline / chainActive.Tip()->nBaseTarget) * 1000 * 1000);
    auto now = GetTimeMicros();
    //if (dl <= now) { //createnewblock now
    CreateNewBlock(script);
    //} else { // start time
    //    dl = (dl - now) / 1000 / 1000;
    //}
    return true;
}

void CPOCBlockAssember::CreateNewBlock(const CScript& scriptPubKeyIn)
{
    auto params = Params();
    auto blk = BlockAssembler(params).CreateNewBlock(scriptPubKeyIn, nonce, plotID, deadline);
    if (blk) {
        uint32_t extraNonce = 0;
        IncrementExtraNonce(&blk->block, chainActive.Tip(), extraNonce);
        auto pblk = std::make_shared<CBlock>(blk->block);
        if (ProcessNewBlock(params, pblk, true, NULL) == false) {
            LogPrintf("ProcessNewBlock failed");
        }
    } else {
        //TODO.. log error
    }
}