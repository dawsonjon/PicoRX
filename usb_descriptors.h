#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#define TUD_AUDIO_PICORX_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN\
  + TUD_AUDIO20_DESC_STD_AC_LEN\
  + TUD_AUDIO20_DESC_CS_AC_LEN\
  + TUD_AUDIO20_DESC_CLK_SRC_LEN\
  + TUD_AUDIO20_DESC_INPUT_TERM_LEN\
  + TUD_AUDIO20_DESC_OUTPUT_TERM_LEN\
  + TUD_AUDIO20_DESC_FEATURE_UNIT_LEN(2)\
  + TUD_AUDIO20_DESC_STD_AS_LEN\
  + TUD_AUDIO20_DESC_STD_AS_LEN\
  + TUD_AUDIO20_DESC_CS_AS_INT_LEN\
  + TUD_AUDIO20_DESC_TYPE_I_FORMAT_LEN\
  + TUD_AUDIO20_DESC_STD_AS_ISO_EP_LEN\
  + TUD_AUDIO20_DESC_CS_AS_ISO_EP_LEN)

#define TUD_AUDIO_PICORX_DESCRIPTOR(_itfnum, _stridx, _nBytesPerSample, _nBitsUsedPerSample, _epin, _epsize) \
  /* Standard Interface Association Descriptor (IAD) */\
  TUD_AUDIO20_DESC_IAD(/*_firstitf*/ _itfnum, /*_nitfs*/ 0x02, /*_stridx*/ 0x00),\
  /* Standard AC Interface Descriptor(4.7.1) */\
  TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ _itfnum, /*_nEPs*/ 0x00, /*_stridx*/ _stridx),\
  /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
  TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_MICROPHONE, /*_totallen*/ TUD_AUDIO20_DESC_CLK_SRC_LEN+TUD_AUDIO20_DESC_INPUT_TERM_LEN+TUD_AUDIO20_DESC_OUTPUT_TERM_LEN+TUD_AUDIO20_DESC_FEATURE_UNIT_LEN(2), /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
  /* Clock Source Descriptor(4.7.2.1) */\
  TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ 0x04, /*_attr*/ AUDIO20_CLOCK_SOURCE_ATT_INT_FIX_CLK, /*_ctrl*/ (AUDIO20_CTRL_R << AUDIO20_CLOCK_SOURCE_CTRL_CLK_FRQ_POS), /*_assocTerm*/ 0x01,  /*_stridx*/ 0x00),\
  /* Input Terminal Descriptor(4.7.2.4) */\
  TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ 0x01, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x03, /*_clkid*/ 0x04, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS, /*_stridx*/ 0x00),\
  /* Output Terminal Descriptor(4.7.2.5) */\
  TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ 0x03, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x01, /*_srcid*/ 0x02, /*_clkid*/ 0x04, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
  /* Feature Unit Descriptor(4.7.2.8) */\
  TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ 0x02, /*_srcid*/ 0x01, /*_stridx*/ 0x00,/*_ctrlch0master*/ AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch1*/ AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS, /*_ctrlch2*/ AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
  TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ 0x00),\
  /* Standard AS Interface Descriptor(4.9.1) */\
  /* Interface 1, Alternate 1 - alternate interface for data streaming */\
  TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum)+1), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ 0x00),\
  /* Class-Specific AS Interface Descriptor(4.9.2) */\
  TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ 0x03, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
  /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
  TUD_AUDIO20_DESC_TYPE_I_FORMAT(_nBytesPerSample, _nBitsUsedPerSample),\
  /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
  TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ _epsize, /*_interval*/ 0x01),\
  /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
  TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

#endif
