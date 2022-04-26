
#ifndef SRSGNB_MAC_RACH_HANDLER_H
#define SRSGNB_MAC_RACH_HANDLER_H

#include "srsgnb/ran/du_types.h"
#include "srsgnb/ran/slot_point.h"

namespace srsgnb {

using timing_advance_report = uint16_t;

struct mac_rach_indication {
  du_cell_index_t cell_index;
  slot_point      slot_rx;
  /// Index of the first OFDM Symbol where RACH was detected.
  unsigned symbol_index;
  unsigned frequency_index;
  unsigned preamble_id;
  unsigned timing_advance;
};

class mac_rach_handler
{
public:
  virtual ~mac_rach_handler()                                              = default;
  virtual void handle_rach_indication(const mac_rach_indication& rach_ind) = 0;
};

} // namespace srsgnb

#endif // SRSGNB_MAC_RACH_HANDLER_H
