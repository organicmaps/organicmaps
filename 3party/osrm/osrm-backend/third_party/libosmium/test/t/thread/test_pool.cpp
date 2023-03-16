#include "catch.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

#include <osmium/thread/pool.hpp>

static std::atomic<int> result;

struct test_job_ok {
    void operator()() const {
        result = 1;
    }
};

struct test_job_with_result {
    int operator()() const {
        return 42;
    }
};

struct test_job_throw {
    void operator()() const {
        throw std::runtime_error("exception in pool thread");
    }
};

TEST_CASE("thread") {

    SECTION("can get access to thread pool") {
        auto& pool = osmium::thread::Pool::instance();
        REQUIRE(pool.queue_empty());
    }

    SECTION("can send job to thread pool") {
        auto& pool = osmium::thread::Pool::instance();
        result = 0;
        auto future = pool.submit(test_job_ok {});

        // wait a bit for the other thread to get a chance to run
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        REQUIRE(result == 1);

        future.get();

        REQUIRE(true);
    }

    SECTION("can send job to thread pool") {
        auto& pool = osmium::thread::Pool::instance();
        auto future = pool.submit(test_job_with_result {});

        REQUIRE(future.get() == 42);
    }

    SECTION("can throw from job in thread pool") {
        auto& pool = osmium::thread::Pool::instance();
        result = 0;

        bool got_exception = false;
        auto future = pool.submit(test_job_throw {});

        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }

}

