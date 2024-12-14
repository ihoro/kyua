// Copyright 2024 The Kyua Authors.
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

#include "cli/cmd_rr.hpp"

#include "cli/common.ipp"
#include "engine/rr/rr.hpp"
#include "utils/format/macros.hpp"

namespace cmdline = utils::cmdline;
namespace config = utils::config;
namespace rr = engine::rr;

using cli::cmd_rr;


/// Default constructor for cmd_rr.
cmd_rr::cmd_rr(void) : cli_command(
    "rr", "[resolver-name ...]", 0, -1, "Run requirement resolvers")
{
    //add_option(build_root_option);
    //add_option(kyuafile_option);
}


/// Entry point for the "rr" subcommand.
///
/// \param ui Object to interact with the I/O of the program.
/// \param cmdline Representation of the command line to the subcommand.
/// \param user_config The runtime configuration of the program.
///
/// \return 0 if successful, 1 otherwise.
int
cmd_rr::run(cmdline::ui* ui, const cmdline::parsed_cmdline& cmdline,
            const config::tree& user_config)
{
    // List available resolvers
    if (cmdline.arguments().empty()) {
        for (auto r : rr::resolvers()) {
            ui->out(r->name(), false);
            ui->out("\t\t\t", false);
            ui->out(r->description());
        }
        return EXIT_SUCCESS;
    }

    // Or run specified ones
    int error = EXIT_SUCCESS;
    for (auto rname : cmdline.arguments()) {
        std::shared_ptr< rr::interface > resolver = nullptr;
        for (auto r : rr::resolvers())
            if (r->name() == rname) {
                resolver = r;
                break;
            }
        if (resolver == nullptr) {
            ui->out(F("Unknown requirement resolver: %s") % rname);
            return EXIT_FAILURE;
        }
        error = resolver->exec(ui, cmdline, user_config);
        if (error != EXIT_SUCCESS)
            break;
    }

    return error;
}
