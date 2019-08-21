#include <blockcache.h>
#include <util/time.h>
#include <chain.h>
#include <logging.h>

CBlockCache::CBlockCache():prevIndex(nullptr) {}

void CBlockCache::UpdateBestBlockIndex(const CBlockIndex* index)
{
    boost::lock_guard<boost::mutex> lock(mtx);
    if (!blocks.empty()) {
        LogPrintf("%s: active chain update block, cache remove %d blocks\n", __func__, blocks.size());
        blocks.clear();
    }
    prevIndex = index;
}

void CBlockCache::AddBlock(const std::shared_ptr<const CBlock>& blk, std::function<bool()> const &func)
{
    boost::lock_guard<boost::mutex> lock(mtx);
    if (blk->hashPrevBlock != prevIndex->GetBlockHash()){
        LogPrintf("%s: AddBlock in too far away, discard from cache, block:%s", __func__, blk->GetHash());
        return;
    }
    blocks.emplace_back(std::make_shared<CBlock>(*blk));
    static auto compare = [](const std::shared_ptr<const CBlock> blk1, const std::shared_ptr<const CBlock> blk2)->bool {
        return blk1->nDeadline < blk2->nDeadline;
    };
    std::sort(blocks.begin(), blocks.end(), compare);
    handle = func;
}

void CBlockCache::PushBlock()
{
    boost::lock_guard<boost::mutex> lock(mtx);
    if (blocks.empty()) return;
    auto block = blocks[0];
    auto dl = block->nDeadline / prevIndex->nBaseTarget;
    if (GetSystemTimeInSeconds() >= dl + prevIndex->nTime) { //accept best chain
        //pop block
        LogPrintf("%s: accpet active chain block, block:%s\n", __func__, block->GetHash().ToString());
        handle();
        blocks.clear();
    }
}

std::unique_ptr<CBlockCache> g_blockCache;