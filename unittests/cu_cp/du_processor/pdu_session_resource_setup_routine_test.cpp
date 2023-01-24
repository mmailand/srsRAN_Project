/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_processor_routine_manager_test_helpers.h"
#include "srsgnb/support/async/async_test_utils.h"
#include "srsgnb/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsgnb;
using namespace srs_cu_cp;

/// Note: Check if UE ID is valid is done by caller. Injection of invalid cu_cp_ue_id results in assertion.

class pdu_session_resource_setup_test : public du_processor_routine_manager_test
{
protected:
  void start_procedure(const cu_cp_pdu_session_resource_setup_request& msg,
                       bool                                            bearer_context_setup_outcome,
                       bool                                            ue_context_modification_outcome,
                       bool                                            bearer_context_modification_outcome,
                       bool                                            rrc_reconfiguration_outcome)
  {
    e1ap_ctrl_notifier.set_bearer_context_setup_outcome(bearer_context_setup_outcome);
    f1ap_ue_ctxt_notifier.set_ue_context_modification_outcome(ue_context_modification_outcome);
    e1ap_ctrl_notifier.set_bearer_context_modification_outcome(bearer_context_modification_outcome);
    rrc_ue_ctrl_notifier.set_rrc_reconfiguration_outcome(rrc_reconfiguration_outcome);

    t = routine_mng->start_pdu_session_resource_setup_routine(
        msg, security_cfg, rrc_ue_ctrl_notifier, rrc_ue_drb_manager);
    t_launcher.emplace(t);
  }

  bool was_pdu_session_resource_setup_successful() const
  {
    if (not t.ready()) {
      return false;
    }

    if (t.get().pdu_session_res_failed_to_setup_items.size() > 0) {
      return false;
    }

    return t.get().pdu_session_res_setup_response_items.size() > 0;
  }

  async_task<cu_cp_pdu_session_resource_setup_response>                   t;
  optional<lazy_task_launcher<cu_cp_pdu_session_resource_setup_response>> t_launcher;
};

TEST_F(pdu_session_resource_setup_test, when_bearer_context_setup_failure_received_then_setup_fails)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = generate_pdu_session_resource_setup();

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, false, false, false, false);

  // nothing has failed to setup
  ASSERT_FALSE(was_pdu_session_resource_setup_successful());
}

TEST_F(pdu_session_resource_setup_test, when_ue_context_modification_failure_received_then_setup_fails)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = generate_pdu_session_resource_setup();

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, true, false, false, false);

  // nothing has failed to setup
  ASSERT_FALSE(was_pdu_session_resource_setup_successful());
}

TEST_F(pdu_session_resource_setup_test, when_bearer_context_modification_failure_received_then_setup_fails)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = generate_pdu_session_resource_setup();

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, true, true, false, false);

  // nothing has failed to setup
  ASSERT_FALSE(was_pdu_session_resource_setup_successful());
}

TEST_F(pdu_session_resource_setup_test, when_rrc_reconfiguration_fails_then_setup_fails)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = generate_pdu_session_resource_setup();

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, true, true, true, false);

  // nothing has failed to setup
  ASSERT_FALSE(was_pdu_session_resource_setup_successful());
}

TEST_F(pdu_session_resource_setup_test, when_rrc_reconfiguration_succeeds_then_setup_succeeds)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = generate_pdu_session_resource_setup();

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, true, true, true, true);

  // nothing has failed to setup
  ASSERT_TRUE(was_pdu_session_resource_setup_successful());
}

/// Test handling of PDU session setup request without any setup item.
TEST_F(pdu_session_resource_setup_test, when_empty_pdu_session_setup_request_received_then_setup_fails)
{
  // Test Preamble.
  cu_cp_pdu_session_resource_setup_request request = {}; // empty message
  request.cu_cp_ue_id                              = uint_to_cu_cp_ue_id(0);

  // Start PDU SESSION RESOURCE SETUP routine.
  this->start_procedure(request, true, true, true, true);

  // it should be ready immediately
  ASSERT_TRUE(t.ready());

  // Nothing has been set up or failed
  ASSERT_FALSE(was_pdu_session_resource_setup_successful());
}
