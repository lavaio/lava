#include "assember.h"
#include <boost/bind.hpp>
#include "chainparams.h"
#include "logging.h"
#include "miner.h"
#include "poc.h"
#include "util/time.h"
#include "validation.h"

CPOCBlockAssember::CPOCBlockAssember()
    : scheduler(std::make_shared<CScheduler>())
{
    setNull();
    auto f = boost::bind(&CScheduler::serviceQueue, scheduler.get());
    thread = std::make_shared<boost::thread>(f);
    const auto interval = 200;
    scheduler->scheduleEvery(std::bind(&CPOCBlockAssember::checkDeadline, this), interval);
}

CPOCBlockAssember::~CPOCBlockAssember()
{
    thread->interrupt();
}

bool CPOCBlockAssember::UpdateDeadline(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script)
{
    auto indexPrev = chainActive.Tip();
    
    if (indexPrev->nHeight != (height - 1)) {
        LogPrintf("chainActive has been update, the new index is %uul, but the height to be produced is %uul\n", indexPrev->nHeight, height);
		return false;
	}
    auto params = Params();
    if (deadline / indexPrev->nBaseTarget > params.TargetDeadline()) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    if (this->deadline != 0 && deadline >= this->deadline) {
        LogPrintf("Invalid deadline %uul\n", deadline);
        return false;
    }

    auto generationSignature = CalcGenerationSignature(indexPrev->genSign, indexPrev->nPlotID);
    if (CalcDeadline(generationSignature, indexPrev->nHeight + 1, plotID, nonce) != deadline) {
        LogPrintf("Deadline inconformity %uul\n", deadline);
        return false;
    }
    auto ts = (deadline / indexPrev->nBaseTarget);
    LogPrintf("Update new deadline: %u, now: %u, target: %u\n", ts, GetTimeMillis() / 1000, indexPrev->nTime + ts);
    SetAssemberItems(height, plotID, nonce, deadline, script);
    return true;
}

void CPOCBlockAssember::CreateNewBlock(const CScript& scriptPubKeyIn)
{
    auto item = AssemberItems();
    LogPrintf("CPOCBlockAssember CreateNewBlock, plotid: %u nonce:%u newheight:%u deadline:%u utc:%u\n", item.plotID, item.nonce, item.height, item.deadline, GetTimeMillis()/1000);
    auto params = Params();
    auto blk = BlockAssembler(params).CreateNewBlock(scriptPubKeyIn, item.nonce, item.plotID, item.deadline);
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

void CPOCBlockAssember::checkDeadline()
{
    LOCK(cs_main);
    if (deadline == 0)
        return;
    auto lastBlockTime = chainActive.Tip()->GetBlockHeader().GetBlockTime();
    auto ts = ((deadline / chainActive.Tip()->nBaseTarget) * 1000);
    //check the chainindex tip is 1 height lower than newBlockHeight input;
    //if not, that means the plotid, dl, nonce are all not for this chain tip to produce new block!
    if (chainActive.Tip()->nHeight != (height-1)) {
        LogPrintf("AssemberInfo Mismatch: these plotit, deadline and nonce are all for producing the BlockHeight %u, but here the function wants to produce the %u height Block!", height, chainActive.Tip()->nHeight);
        setNull();
		return;
    }
    auto dl = (lastBlockTime * 1000) + ts;
    if (GetTimeMillis() >= dl) {
        CreateNewBlock(scriptPubKeyIn);
        setNull();
    }
}

void CPOCBlockAssember::setNull()
{
    height = 0;
	plotID = 0;
    nonce = 0;
    deadline = 0;
    genSig.exchange(uint256());
    scriptPubKeyIn.exchange(CScript());
}

void CPOCBlockAssember::Interrupt()
{
    thread->interrupt();
    thread->join();
    LogPrintf("CPOCBlockAssember exited\n");
}

struct AssemberParams CPOCBlockAssember::AssemberItems()
{
	boost::lock_guard<boost::mutex> lock(mtx);
    auto p = AssemberParams{
        height,
        plotID,
        nonce,
        deadline,
        scriptPubKeyIn};
    return std::move(p);
}

void CPOCBlockAssember::SetAssemberItems(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script)
{
    boost::lock_guard<boost::mutex> lock(mtx);
	this->genSig.exchange(genSig);
    this->height = height;
    this->plotID = plotID;
    this->deadline = deadline;
    this->nonce = nonce;
    this->scriptPubKeyIn = script;
}