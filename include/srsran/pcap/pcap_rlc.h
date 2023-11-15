/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/adt/byte_buffer.h"
#include "srsran/adt/byte_buffer_chain.h"
#include "srsran/ran/du_types.h"
#include "srsran/ran/lcid.h"
#include "srsran/rlc/rlc_config.h"
#include "fmt/format.h"
#include <cstdint>

namespace srsran {

struct pcap_rlc_pdu_context;

/// \brief Interface class for writing a RLC PCAP to a file.
class pcap_rlc
{
public:
  virtual ~pcap_rlc() = default;

  virtual void open(const std::string& filename_)                                          = 0;
  virtual void close()                                                                     = 0;
  virtual bool is_write_enabled() const                                                    = 0;
  virtual void push_pdu(const pcap_rlc_pdu_context& context, const byte_buffer_chain& pdu) = 0;
  virtual void push_pdu(const pcap_rlc_pdu_context& context, const byte_buffer_slice& pdu) = 0;

  virtual void capture_srb(bool srb_enabled_) = 0;
  virtual void capture_drb(bool drb_enabled_) = 0;
};

/// \brief Context information for every RLC NR PDU that will be logged.
///
/// This struct follows the definition in Wireshark's "packet-rlc-nr.h".
struct pcap_rlc_pdu_context {
  uint8_t  rlc_mode;
  uint8_t  direction;
  uint8_t  sequence_number_length;
  uint8_t  bearer_type;
  uint8_t  bearer_id;
  uint16_t ueid;

  ~pcap_rlc_pdu_context() = default;

  /// \brief Create a PCAP PDU context for DL RLC AM PDUs
  /// \param ue_index UE index
  /// \param rb_id Radio bearer type and ID
  /// \param cfg RLC TX AM config
  pcap_rlc_pdu_context(du_ue_index_t ue_index, rb_id_t rb_id, const rlc_tx_am_config& cfg);

  /// \brief Create a PCAP PDU context for UL RLC AM PDUs
  /// \param ue_index UE index
  /// \param rb_id Radio bearer type and ID
  /// \param cfg RLC RX AM config
  pcap_rlc_pdu_context(du_ue_index_t ue_index, rb_id_t rb_id, const rlc_rx_am_config& cfg);

  /// \brief Create a PCAP PDU context for DL RLC UM PDUs
  /// \param ue_index UE index
  /// \param rb_id Radio bearer type and ID
  /// \param cfg RLC TX UM config
  pcap_rlc_pdu_context(du_ue_index_t ue_index, rb_id_t rb_id, const rlc_tx_um_config& cfg);

  /// \brief Create a PCAP PDU context for UL RLC UM PDUs
  /// \param ue_index UE index
  /// \param rb_id Radio bearer type and ID
  /// \param cfg RLC RX UM config
  pcap_rlc_pdu_context(du_ue_index_t ue_index, rb_id_t rb_id, const rlc_rx_um_config& cfg);

  /// \brief Create a PCAP PDU context for UL/DL RLC TM PDUs
  /// \param ue_index UE index
  /// \param rb_id Radio bearer type and ID
  /// \param is_uplink RLC TM direction (DL: false, UL: true)
  pcap_rlc_pdu_context(du_ue_index_t ue_index, rb_id_t rb_id, bool is_uplink);

private:
  /// \brief Sets the SN length according to RLC AM-specific SN length type
  /// \param sn_field_length RLC AM-specific SN length type
  void set_sequence_number_length(rlc_am_sn_size sn_field_length);

  /// \brief Sets the SN length according to RLC UM-specific SN length type
  /// \param sn_field_length RLC UM-specific SN length type
  void set_sequence_number_length(rlc_um_sn_size sn_field_length);

  /// \brief Sets the radio bearer ID and type
  /// \param rb_id Radio bearer ID and type
  void set_bearer_info(rb_id_t rb_id);
};

/// \brief Dummy implementation RLC PCAP for testing
class pcap_rlc_dummy : public pcap_rlc
{
public:
  ~pcap_rlc_dummy() = default;
  pcap_rlc_dummy()  = default;

  void open(const std::string& filename_) override {}
  void close() override {}
  bool is_write_enabled() const override { return false; }
  void push_pdu(const pcap_rlc_pdu_context& context, const byte_buffer_chain& pdu) override {}
  void push_pdu(const pcap_rlc_pdu_context& context, const byte_buffer_slice& pdu) override {}

  void capture_srb(bool srb_enabled_) override {}
  void capture_drb(bool drb_enabled_) override {}
};

// Pre-defined values for data fields of the PCAP PDU context as defined in Wireshark's "packet-rlc-nr.h"

// RLC mode
constexpr uint8_t PCAP_RLC_TM_MODE = 1;
constexpr uint8_t PCAP_RLC_UM_MODE = 2;
constexpr uint8_t PCAP_RLC_AM_MODE = 4;

// Direction
constexpr uint8_t PCAP_RLC_DIRECTION_UPLINK   = 0;
constexpr uint8_t PCAP_RLC_DIRECTION_DOWNLINK = 1;

// Bearer type
constexpr uint8_t PCAP_RLC_BEARER_TYPE_CCCH = 1;
constexpr uint8_t PCAP_RLC_BEARER_TYPE_SRB  = 4;
constexpr uint8_t PCAP_RLC_BEARER_TYPE_DRB  = 5;

// RLC sequence number length
constexpr uint8_t PCAP_RLC_TM_SN_LENGTH_0_BITS  = 0;
constexpr uint8_t PCAP_RLC_UM_SN_LENGTH_6_BITS  = 6;
constexpr uint8_t PCAP_RLC_UM_SN_LENGTH_12_BITS = 12;
constexpr uint8_t PCAP_RLC_AM_SN_LENGTH_12_BITS = 12;
constexpr uint8_t PCAP_RLC_AM_SN_LENGTH_18_BITS = 18;

} // namespace srsran

namespace fmt {
template <>
struct formatter<srsran::pcap_rlc_pdu_context> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const srsran::pcap_rlc_pdu_context& pcap_context, FormatContext& ctx)
      -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(),
                     "rlc_mode={} dir={} sn_len={} bearer_type={} bearer_id={} ueid={}",
                     pcap_context.rlc_mode,
                     pcap_context.direction,
                     pcap_context.sequence_number_length,
                     pcap_context.bearer_type,
                     pcap_context.bearer_id,
                     pcap_context.ueid);
  }
};

} // namespace fmt
