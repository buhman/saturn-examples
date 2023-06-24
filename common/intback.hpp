#pragma once

namespace intback {
  enum fsm : uint8_t {
    PORT_STATUS = 0,
    PERIPHERAL_ID,
    DATA1,
    DATA2,
    DATA3,
    DATA4,
    FSM_NEXT
  };

  struct state {
    uint8_t fsm;
    uint8_t controller_ix;
    uint8_t port_ix;
    uint8_t oreg_ix;
  
    uint8_t port_connected;
    uint8_t data_size;
    uint8_t peripheral_type;
    uint8_t kbd_bits;
  };

  typedef void (*keyboard_func_ptr)(uint8_t keysym, uint8_t kbd_bits);
  typedef void (*digital_func_ptr)(uint8_t fsm_state, uint8_t data);

  static struct state state;
  
  inline void fsm(digital_func_ptr digital_cb, keyboard_func_ptr keyboard_cb)
  {
    if ((smpc.reg.SR & SR__PDL) != 0) {
      // PDL == 1; 1st peripheral data
      state.oreg_ix = 0;
      state.controller_ix = 0;
      state.port_ix = 0;
      state.fsm = PORT_STATUS;

      state.port_connected = 0;
      state.data_size = 0;
      state.peripheral_type = 0;
      state.kbd_bits = 0;
    }

    /*
      This intback handling is oversimplified:

      - up to 2 controllers may be (directly) connected
      - multitaps are not parsed correctly
    */
    while (state.oreg_ix < 32) {
      reg8 const& oreg = smpc.reg.OREG[state.oreg_ix++].val;
      switch (state.fsm) {
      case PORT_STATUS:
	state.port_connected = (PORT_STATUS__CONNECTORS(oreg) == 1);
	if (state.port_connected) {
	  if (PORT_STATUS__MULTITAP_ID(oreg) != 0xf) {
	    // this port is not directly connected; abort parse:
	    goto abort;
	  }
	} else {
	  state.fsm = FSM_NEXT;
	}
	break;
      case PERIPHERAL_ID:
	state.peripheral_type = PERIPHERAL_ID__TYPE(oreg);
	state.data_size = PERIPHERAL_ID__DATA_SIZE(oreg);
	break;
      case DATA1:
	if (digital_cb != nullptr) digital_cb(state.fsm, oreg);
	break;
      case DATA2:
	if (digital_cb != nullptr) digital_cb(state.fsm, oreg);
	break;
      case DATA3:
	state.kbd_bits = oreg & 0b1111;
	break;
      case DATA4:
	if (keyboard_cb != nullptr) keyboard_cb(state.kbd_bits, oreg);
	break;
      default:
	break;
      }

      if ((state.fsm >= PERIPHERAL_ID && state.data_size <= 0) || state.fsm == FSM_NEXT) {
	if (state.port_ix == 1)
	  break;
	else {
	  state.port_ix++;
	  state.controller_ix++;
	  state.fsm = PORT_STATUS;
	}
      } else {
	state.fsm++;
	state.data_size--;
      }
    }

    if ((smpc.reg.SR & SR__NPE) != 0) {
      smpc.reg.IREG[0].val = INTBACK__IREG0__CONTINUE;
    } else {
    abort:
      smpc.reg.IREG[0].val = INTBACK__IREG0__BREAK;
    }
  }
}
