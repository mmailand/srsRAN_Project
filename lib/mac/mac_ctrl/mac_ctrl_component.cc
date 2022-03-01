
#include "mac_ctrl_component.h"
#include "ue_creation_procedure.h"
#include "ue_delete_procedure.h"
#include "ue_reconfiguration_procedure.h"

using namespace srsgnb;

mac_ctrl_component::mac_ctrl_component(mac_common_config_t& cfg_,
                                       mac_ul_configurer&   ul_unit_,
                                       mac_dl_configurer&   dl_unit_) :
  cfg(cfg_), logger(cfg.logger), ul_unit(ul_unit_), dl_unit(dl_unit_)
{
  std::fill(rnti_to_ue_index_map.begin(), rnti_to_ue_index_map.end(), MAX_NOF_UES);
}

void mac_ctrl_component::ue_create_request(const mac_ue_create_request_message& msg)
{
  ue_element* u = add_ue(msg.ue_index, msg.crnti, msg.cell_index);
  if (u == nullptr) {
    log_proc_failure(logger, msg.ue_index, msg.crnti, mac_ue_create_request_procedure::name(), "Invalid parameters.");
    mac_ue_create_request_response_message resp{};
    resp.ue_index   = msg.ue_index;
    resp.cell_index = msg.cell_index;
    resp.result     = false;
    cfg.cfg_notifier.on_ue_create_request_complete(resp);
    return;
  }
  // UE object added to ue_db successfully

  // Enqueue UE create request procedure
  u->ctrl_loop.schedule<mac_ue_create_request_procedure>(msg, cfg, *this, ul_unit, dl_unit);
}

void mac_ctrl_component::ue_delete_request(const mac_ue_delete_request_message& msg)
{
  if (not ue_db.contains(msg.ue_index)) {
    log_proc_failure(logger, msg.ue_index, msg.rnti, mac_ue_delete_procedure::name(), "Inexistent ueId.");
    mac_ue_delete_response_message resp{};
    resp.ue_index = msg.ue_index;
    resp.result   = false;
    cfg.cfg_notifier.on_ue_delete_complete(resp);
    return;
  }
  ue_element& u = ue_db[msg.ue_index];

  // Enqueue UE delete procedure
  u.ctrl_loop.schedule<mac_ue_delete_procedure>(msg, cfg, *this, ul_unit, dl_unit);
}

void mac_ctrl_component::ue_reconfiguration_request(const mac_ue_reconfiguration_request_message& msg)
{
  if (not ue_db.contains(msg.ue_index)) {
    log_proc_failure(logger, msg.ue_index, msg.crnti, mac_ue_reconfiguration_procedure::name(), "Inexistent ueId.");
    mac_ue_reconfiguration_response_message resp{};
    resp.ue_index = msg.ue_index;
    resp.result   = false;
    cfg.cfg_notifier.on_ue_reconfiguration_complete(resp);
    return;
  }
  ue_element& u = ue_db[msg.ue_index];

  // Enqueue UE reconfiguration procedure
  u.ctrl_loop.schedule<mac_ue_reconfiguration_procedure>(msg, cfg, ul_unit, dl_unit);
}

mac_ctrl_component::ue_element*
mac_ctrl_component::add_ue(du_ue_index_t ue_index, rnti_t crnti, du_cell_index_t cell_index)
{
  srsran_assert(crnti != INVALID_RNTI, "Invalid RNTI");
  srsran_assert(ue_index < MAX_NOF_UES, "Invalid ue_index=%d", ue_index);

  if (rnti_to_ue_index_map[crnti % MAX_NOF_UES] < MAX_NOF_UES) {
    // rnti already exists
    return nullptr;
  }

  if (ue_db.contains(ue_index)) {
    // UE already existed with same ue_index
    return nullptr;
  }

  // Create UE object
  ue_db.emplace(ue_index);
  ue_element& u        = ue_db[ue_index];
  u.ue_ctx.du_ue_index = ue_index;
  u.ue_ctx.rnti        = crnti;
  u.ue_ctx.pcell_idx   = cell_index;

  // Update RNTI -> UE index map
  rnti_to_ue_index_map[crnti % MAX_NOF_UES] = ue_index;
  return &u;
}

void mac_ctrl_component::remove_ue(du_ue_index_t ue_index)
{
  // Note: The caller of this function can be a UE procedure. Thus, we have to wait for the procedure to finish
  // before safely removing the UE. This achieved via an async task in the MAC main control loop

  srsran_assert(ue_index < MAX_NOF_UES, "Invalid ueId={}", ue_index);
  if (not ue_db.contains(ue_index)) {
    logger.warning("Failed to find ueId={}", ue_index);
    return;
  }
  logger.debug("Scheduling ueId={} deletion", ue_index);

  // Schedule task removal
  main_ctrl_loop.schedule([this, ue_index](coro_context<async_task<void> >& ctx) {
    CORO_BEGIN(ctx);
    srsran_sanity_check(ue_db.contains(ue_index), "ueId={} was unexpectedly removed", ue_index);

    CORO_AWAIT(ue_db[ue_index].ctrl_loop.request_stop());

    logger.info("Removing ueId={}", ue_index);
    ue_db.erase(ue_index);

    CORO_RETURN();
  });
}

mac_ue_context* mac_ctrl_component::find_ue(du_ue_index_t ue_index)
{
  srsran_assert(ue_index < MAX_NOF_UES, "Invalid ue_index=%d", ue_index);
  return ue_db.contains(ue_index) ? &ue_db[ue_index].ue_ctx : nullptr;
}

mac_ue_context* mac_ctrl_component::find_by_rnti(rnti_t rnti)
{
  srsran_assert(rnti != INVALID_RNTI, "Invalid rnti=0x%x", rnti);
  du_ue_index_t ue_index = rnti_to_ue_index_map[rnti % MAX_NOF_UES];
  if (ue_index == MAX_NOF_UES) {
    return nullptr;
  }
  return find_ue(ue_index);
}
