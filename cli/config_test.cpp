// Copyright 2011 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <fstream>

#include <atf-c++.hpp>

#include "cli/config.hpp"
#include "engine/exceptions.hpp"
#include "engine/user_files/config.hpp"
#include "utils/env.hpp"
#include "utils/fs/operations.hpp"
#include "utils/fs/path.hpp"

namespace cmdline = utils::cmdline;
namespace fs = utils::fs;
namespace user_files = engine::user_files;


namespace {


/// Creates a configuration file for testing purposes.
///
/// To ensure that the loaded file is the one created by this function, use
/// validate_mock_config().
///
/// \param name The name of the configuration file to create.
/// \param cookie The magic value to set in the configuration file, or NULL if a
///     broken configuration file is desired.
static void
create_mock_config(const char* name, const char* cookie)
{
    std::ofstream output(name);
    ATF_REQUIRE(output);
    if (cookie != NULL) {
        output << "syntax('config', 1)\n";
        output << "test_suites.suite.magic_value = '" << cookie << "'\n";
    } else {
        output << "syntax('invalid-file', 1)\n";
    }
}


/// Creates an invalid system configuration.
///
/// \param cookie The magic value to set in the configuration file, or NULL if a
///     broken configuration file is desired.
static void
mock_system_config(const char* cookie)
{
    fs::mkdir(fs::path("system-dir"), 0755);
    utils::setenv("KYUA_CONFDIR", (fs::current_path() / "system-dir").str());
    create_mock_config("system-dir/kyua.conf", cookie);
}


/// Creates an invalid user configuration.
///
/// \param cookie The magic value to set in the configuration file, or NULL if a
///     broken configuration file is desired.
static void
mock_user_config(const char* cookie)
{
    fs::mkdir(fs::path("user-dir"), 0755);
    fs::mkdir(fs::path("user-dir/.kyua"), 0755);
    utils::setenv("HOME", (fs::current_path() / "user-dir").str());
    create_mock_config("user-dir/.kyua/kyua.conf", cookie);
}


/// Ensures that a loaded configuration was created with create_mock_config().
///
/// \param config The configuration to validate.
/// \param cookie The magic value to expect in the configuration file.
static void
validate_mock_config(const user_files::config& config, const char* cookie)
{
    const user_files::properties_map& properties = config.test_suite("suite");
    const user_files::properties_map::const_iterator iter =
        properties.find("magic_value");
    ATF_REQUIRE(iter != properties.end());
    ATF_REQUIRE_EQ(cookie, (*iter).second);
}


}  // anonymous namespace


ATF_TEST_CASE_WITHOUT_HEAD(load_config__none);
ATF_TEST_CASE_BODY(load_config__none)
{
    utils::setenv("KYUA_CONFDIR", "/the/system/does/not/exist");
    utils::setenv("HOME", "/the/user/does/not/exist");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    ATF_REQUIRE(config.test_suites.empty());
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__explicit__ok);
ATF_TEST_CASE_BODY(load_config__explicit__ok)
{
    mock_system_config(NULL);
    mock_user_config(NULL);

    create_mock_config("test-file", "hello");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back("test-file");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    validate_mock_config(config, "hello");
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__explicit__disable);
ATF_TEST_CASE_BODY(load_config__explicit__disable)
{
    mock_system_config(NULL);
    mock_user_config(NULL);

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back("none");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    ATF_REQUIRE(user_files::config::defaults() == config);
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__explicit__fail);
ATF_TEST_CASE_BODY(load_config__explicit__fail)
{
    mock_system_config("ok1");
    mock_user_config("ok2");

    create_mock_config("test-file", NULL);

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back("test-file");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    ATF_REQUIRE_THROW_RE(engine::error, "invalid-file",
                         cli::load_config(mock_cmdline));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__user__ok);
ATF_TEST_CASE_BODY(load_config__user__ok)
{
    mock_system_config(NULL);
    mock_user_config("I am the user config");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    validate_mock_config(config, "I am the user config");
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__user__fail);
ATF_TEST_CASE_BODY(load_config__user__fail)
{
    mock_system_config("valid");
    mock_user_config(NULL);

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    ATF_REQUIRE_THROW_RE(engine::error, "invalid-file",
                         cli::load_config(mock_cmdline));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__user__bad_home);
ATF_TEST_CASE_BODY(load_config__user__bad_home)
{
    mock_system_config("Fallback system config");
    utils::setenv("HOME", "");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    validate_mock_config(config, "Fallback system config");
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__system__ok);
ATF_TEST_CASE_BODY(load_config__system__ok)
{
    mock_system_config("I am the system config");
    utils::setenv("HOME", "/the/user/does/not/exist");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    validate_mock_config(config, "I am the system config");
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__system__fail);
ATF_TEST_CASE_BODY(load_config__system__fail)
{
    mock_system_config(NULL);
    utils::setenv("HOME", "/the/user/does/not/exist");

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    ATF_REQUIRE_THROW_RE(engine::error, "invalid-file",
                         cli::load_config(mock_cmdline));
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__overrides__no);
ATF_TEST_CASE_BODY(load_config__overrides__no)
{
    utils::setenv("KYUA_CONFDIR", fs::current_path().str());

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    options["variable"].push_back("architecture=1");
    options["variable"].push_back("platform=2");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    ATF_REQUIRE_EQ("1", config.architecture);
    ATF_REQUIRE_EQ("2", config.platform);
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__overrides__yes);
ATF_TEST_CASE_BODY(load_config__overrides__yes)
{
    std::ofstream output("config");
    output << "syntax('config', 1)\n";
    output << "architecture = 'do not see me'\n";
    output << "platform = 'see me'\n";
    output.close();

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back("config");
    options["variable"].push_back("architecture=overriden");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    const user_files::config config = cli::load_config(mock_cmdline);
    ATF_REQUIRE_EQ("overriden", config.architecture);
    ATF_REQUIRE_EQ("see me", config.platform);
}


ATF_TEST_CASE_WITHOUT_HEAD(load_config__overrides__fail);
ATF_TEST_CASE_BODY(load_config__overrides__fail)
{
    utils::setenv("KYUA_CONFDIR", fs::current_path().str());

    std::map< std::string, std::vector< std::string > > options;
    options["config"].push_back(cli::config_option.default_value());
    options["variable"].push_back(".a=d");
    const cmdline::parsed_cmdline mock_cmdline(options, cmdline::args_vector());

    ATF_REQUIRE_THROW_RE(engine::error, "Empty test suite.*'\\.a=d'",
                         cli::load_config(mock_cmdline));
}


ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, load_config__none);
    ATF_ADD_TEST_CASE(tcs, load_config__explicit__ok);
    ATF_ADD_TEST_CASE(tcs, load_config__explicit__disable);
    ATF_ADD_TEST_CASE(tcs, load_config__explicit__fail);
    ATF_ADD_TEST_CASE(tcs, load_config__user__ok);
    ATF_ADD_TEST_CASE(tcs, load_config__user__fail);
    ATF_ADD_TEST_CASE(tcs, load_config__user__bad_home);
    ATF_ADD_TEST_CASE(tcs, load_config__system__ok);
    ATF_ADD_TEST_CASE(tcs, load_config__system__fail);
    ATF_ADD_TEST_CASE(tcs, load_config__overrides__no);
    ATF_ADD_TEST_CASE(tcs, load_config__overrides__yes);
    ATF_ADD_TEST_CASE(tcs, load_config__overrides__fail);
}
