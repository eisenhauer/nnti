/**
//@HEADER
// ************************************************************************
//
//                   Trios: Trilinos I/O Support
//                 Copyright 2011 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//Questions? Contact Ron A. Oldfield (raoldfi@sandia.gov)
//
// *************************************************************************
//@HEADER
 */
/**  @file opregistration-server.cpp
 *
 *   @brief Example data transfer server.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 */

/**
 * @defgroup opregistration_server Data Transfer Server
 *
 * @ingroup opregistration_example
 *
 * @{
 */

#include "Trios_config.h"
#include "Trios_nssi_client.h"
#include "Trios_nssi_server.h"
#include "Trios_logger.h"
#include "Trios_timer.h"
#include "Trios_nssi_debug.h"
#include "Trios_nssi_fprint_types.h"

#include "opregistration_service_args.h"

#include <iostream>
#include <string>

#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>



extern log_level opregistration_debug_level;

extern "C" {
uint64_t calc_checksum (const char * buf, const uint64_t size);
}


/**
 * @brief Emulate a read operation where the bulk data is sent using
 *        the \ref nssi_put function.
 *
 * Transfer an array of data structures to the client using the data
 * channel.  This procedure passes the length of the array in the arguments.
 * The server then ``puts'' the unencoded data into the client memory using
 * the \ref nssi_put_data function.  This method evaluates the RDMA
 * transfer performance for \ref nssi_put_data.
 *
 * @param request_id   ID of the request.
 * @param caller      The process ID of the calling process.
 * @param args        Arguments passed with the request.
 * @param data_addr   The remote memory descriptor for the data (not used).
 * @param res_addr    The remote memory descriptor for the result.
 */
class opregistration_request_srvr : public NssiRpc {
    public:
        int doRPC(
                const unsigned long  request_id,
                const NNTI_peer_t   *caller,
                const void          *void_args,
                const NNTI_buffer_t *data_addr,
                const NNTI_buffer_t *res_addr)
        {
            int rc;
            log_level debug_level           = opregistration_debug_level;
            const opregistration_args *args = static_cast<const opregistration_args*>(void_args);

            /* process array (nothing to do) */
            log_debug(debug_level, "starting opregistration_request_srvr->dispatch()");

            log_debug(debug_level, "args->data.int_val(%d) args->data.float_val(%f) args->data.double_val(%f) args->chksum(%lu)",
                    args->data.int_val, args->data.float_val, args->data.double_val, args->chksum);

            /* checksum the data */
            if (args->chksum != calc_checksum((const char*)&args->data, sizeof(args->data))) {
                log_error(debug_level, "client checksum (%lu) != calculated checksum (%lu)",
                        args->chksum, calc_checksum((const char*)&args->data, sizeof(args->data)));
                rc=NNTI_EBADRPC;
            }

            rc = nssi_send_result(caller, request_id, rc, NULL, res_addr);

            return rc;
        }
        void registerRPC()
        {
            NSSI_REGISTER_SERVER_OBJ(OPREGISTRATION_REQUEST_OP, this, opregistration_args, void);
        }
};


/**
 * @brief The NSSI opregistration-server.
 *
 * NSSI has already been initialized and the client already knows the URL of the
 * server.  This function simply registers the server methods and starts the
 * service loop.   The client will send a request to kill the service upon completion.
 *
 */
int opregistration_cpp_server_main(nssi_rpc_transport transport, MPI_Comm server_comm)
{
    int rc = NSSI_OK;

    nssi_service opregistration_svc;
    int server_rank;

    opregistration_request_srvr *opreg_srvr_obj = new opregistration_request_srvr();

    MPI_Comm_rank(server_comm, &server_rank);

    /* options that can be overriden by the command-line */
    std::string server_url(NSSI_URL_LEN, '\0');          /* NNTI-style url of the server */
    std::string logfile("");


    memset(&opregistration_svc, 0, sizeof(nssi_service));


    /* initialize the nssi service */
    rc = nssi_service_init(transport, NSSI_SHORT_REQUEST_SIZE, &opregistration_svc);
    if (rc != NSSI_OK) {
        log_error(opregistration_debug_level, "could not init opregistration_svc: %s",
                nssi_err_str(rc));
        return -1;
    }

    // register objects for the service methods
    opreg_srvr_obj->registerRPC();


    // Get the Server URL
    std::string url(NSSI_URL_LEN, '\0');
    nssi_get_url(transport, &url[0], NSSI_URL_LEN);


    // Set the maxumum number of requests to handle (-1 == infinite)
    opregistration_svc.max_reqs = -1;

    log_debug(opregistration_debug_level, "Starting Server: url = %s", url.c_str());

    // Tell the NSSI server to output log data
    //rpc_debug_level = opregistration_debug_level;

    // start processing requests, the client will send a request to exit when done
    rc = nssi_service_start(&opregistration_svc);
    if (rc != NSSI_OK) {
        log_info(opregistration_debug_level, "exited opregistration_svc: %s",
                nssi_err_str(rc));
    }

    sleep(5);

    /* shutdown the opregistration_svc */
    log_debug(opregistration_debug_level, "shutting down service library");
    nssi_service_fini(&opregistration_svc);


    return rc;
}

/**
 * @}
 */
