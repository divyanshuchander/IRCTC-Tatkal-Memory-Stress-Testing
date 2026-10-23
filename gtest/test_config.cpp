#include <gtest/gtest.h>

extern "C" {
#include "config.h"
}

TEST(ConfigParseTest, DefaultSeats)
{
    char program[] = "test_config";
    char *argv[] = {program, nullptr};
    config parsed_config{};
    ASSERT_EQ(config_parse(1, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.seats, 10);
}

TEST(ConfigParseTest, DefaultRequests)
{
    char program[] = "test_config";
    char *argv[] = {program, nullptr};
    config parsed_config{};
    ASSERT_EQ(config_parse(1, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.requests, 100);
}

TEST(ConfigParseTest, DefaultWorkers)
{
    char program[] = "test_config";
    char *argv[] = {program, nullptr};
    config parsed_config{};
    ASSERT_EQ(config_parse(1, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.workers, 4);
}

TEST(ConfigParseTest, RejectsUnknownOption)
{
    char program[] = "test_config";
    char option[] = "--unknown";
    char value[] = "10";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 0);
}