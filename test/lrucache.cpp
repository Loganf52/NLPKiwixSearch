/*
 * Copyright (c) 2014, lamerman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of lamerman nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "../src/tools/lrucache.h"
#include "../src/tools/concurrent_cache.h"
#include "gtest/gtest.h"

const unsigned int NUM_OF_TEST2_RECORDS = 100;
const unsigned int TEST2_CACHE_CAPACITY = 50;

TEST(CacheTest, SimplePut) {
    kiwix::lru_cache<int, int> cache_lru(1);
    cache_lru.put(7, 777);
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(777, cache_lru.get(7));
    EXPECT_EQ(1U, cache_lru.size());
}

TEST(CacheTest, OverwritingPut) {
    kiwix::lru_cache<int, int> cache_lru(1);
    cache_lru.put(7, 777);
    cache_lru.put(7, 222);
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(222, cache_lru.get(7));
    EXPECT_EQ(1U, cache_lru.size());
}

TEST(CacheTest, MissingValue) {
    kiwix::lru_cache<int, int> cache_lru(1);
    EXPECT_TRUE(cache_lru.get(7).miss());
    EXPECT_FALSE(cache_lru.get(7).hit());
    EXPECT_THROW(cache_lru.get(7).value(), std::range_error);
}

TEST(CacheTest, DropValue) {
    kiwix::lru_cache<int, int> cache_lru(3);
    cache_lru.put(7, 777);
    cache_lru.put(8, 888);
    cache_lru.put(9, 999);
    EXPECT_EQ(3U, cache_lru.size());
    EXPECT_TRUE(cache_lru.exists(7));
    EXPECT_EQ(777, cache_lru.get(7));

    EXPECT_TRUE(cache_lru.drop(7));

    EXPECT_EQ(2U, cache_lru.size());
    EXPECT_FALSE(cache_lru.exists(7));
    EXPECT_THROW(cache_lru.get(7).value(), std::range_error);

    EXPECT_FALSE(cache_lru.drop(7));
}

TEST(CacheTest1, KeepsAllValuesWithinCapacity) {
    kiwix::lru_cache<int, int> cache_lru(TEST2_CACHE_CAPACITY);

    for (unsigned int i = 0; i < NUM_OF_TEST2_RECORDS; ++i) {
        cache_lru.put(i, i);
    }

    for (unsigned int i = 0; i < NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY; ++i) {
        EXPECT_FALSE(cache_lru.exists(i));
    }

    for (unsigned int i = NUM_OF_TEST2_RECORDS - TEST2_CACHE_CAPACITY; i < NUM_OF_TEST2_RECORDS; ++i) {
        EXPECT_TRUE(cache_lru.exists(i));
        EXPECT_EQ((int)i, cache_lru.get(i));
    }

    size_t size = cache_lru.size();
    EXPECT_EQ(TEST2_CACHE_CAPACITY, size);
}

TEST(ConcurrentCacheTest, handleException) {
    kiwix::ConcurrentCache<int, int> cache(1);
    auto val = cache.getOrPut(7, []() { return 777; });
    EXPECT_EQ(val, 777);
    EXPECT_THROW(cache.getOrPut(8, []() { throw std::runtime_error("oups"); return 0; }), std::runtime_error);
    val = cache.getOrPut(8, []() { return 888; });
    EXPECT_EQ(val, 888);
}

TEST(ConcurrentCacheTest, weakPtr) {
    kiwix::ConcurrentCache<int, std::shared_ptr<int>> cache(1);
    auto refValue = cache.getOrPut(7, []() { return std::make_shared<int>(777); });
    EXPECT_EQ(*refValue, 777);
    EXPECT_EQ(refValue.use_count(), 2);

    // This will drop shared(777) from the cache
    cache.getOrPut(8, []() { return std::make_shared<int>(888); });
    EXPECT_EQ(refValue.use_count(), 1);

    // We must get the shared value from the weakPtr we have
    EXPECT_NO_THROW(cache.getOrPut(7, []() { throw std::runtime_error("oups"); return nullptr; }));
    EXPECT_EQ(refValue.use_count(), 2);

    // Drop all ref
    cache.getOrPut(8, []() { return std::make_shared<int>(888); });
    refValue.reset();

    // Be sure we call the construction function
    EXPECT_THROW(cache.getOrPut(7, []() { throw std::runtime_error("oups"); return nullptr; }), std::runtime_error);
}
