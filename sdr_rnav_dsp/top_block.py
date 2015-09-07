#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Wed May  7 22:21:19 2014
##################################################

from gnuradio import analog
from gnuradio import audio
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import forms
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import osmosdr
import wx

class top_block(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Top Block")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 192000
        self.f_f = f_f = 0
        self.f_c = f_c = 88500000

        ##################################################
        # Blocks
        ##################################################
        _f_f_sizer = wx.BoxSizer(wx.VERTICAL)
        self._f_f_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_f_f_sizer,
        	value=self.f_f,
        	callback=self.set_f_f,
        	label='f_f',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._f_f_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_f_f_sizer,
        	value=self.f_f,
        	callback=self.set_f_f,
        	minimum=-10000,
        	maximum=10000,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_f_f_sizer)
        _f_c_sizer = wx.BoxSizer(wx.VERTICAL)
        self._f_c_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_f_c_sizer,
        	value=self.f_c,
        	callback=self.set_f_c,
        	label='f_c',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._f_c_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_f_c_sizer,
        	value=self.f_c,
        	callback=self.set_f_c,
        	minimum=87900000,
        	maximum=107000000,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_f_c_sizer)
        self.wxgui_waterfallsink2_0 = waterfallsink2.waterfall_sink_c(
        	self.GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=samp_rate,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title="Waterfall Plot",
        )
        self.Add(self.wxgui_waterfallsink2_0.win)
        self.osmosdr_source_0 = osmosdr.source( args="numchan=" + str(1) + " " + "" )
        self.osmosdr_source_0.set_sample_rate(samp_rate)
        self.osmosdr_source_0.set_center_freq(f_c + f_f, 0)
        self.osmosdr_source_0.set_freq_corr(0, 0)
        self.osmosdr_source_0.set_dc_offset_mode(0, 0)
        self.osmosdr_source_0.set_iq_balance_mode(0, 0)
        self.osmosdr_source_0.set_gain_mode(0, 0)
        self.osmosdr_source_0.set_gain(10, 0)
        self.osmosdr_source_0.set_if_gain(20, 0)
        self.osmosdr_source_0.set_bb_gain(20, 0)
        self.osmosdr_source_0.set_antenna("", 0)
        self.osmosdr_source_0.set_bandwidth(0, 0)
          
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate)
        self.audio_sink_0 = audio.sink(32000, "", True)
        self.analog_wfm_rcv_0 = analog.wfm_rcv(
        	quad_rate=500000,
        	audio_decimation=samp_rate/32000,
        )

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_throttle_0, 0), (self.wxgui_waterfallsink2_0, 0))
        self.connect((self.analog_wfm_rcv_0, 0), (self.audio_sink_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.analog_wfm_rcv_0, 0))
        self.connect((self.osmosdr_source_0, 0), (self.blocks_throttle_0, 0))


# QT sink close method reimplementation

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.wxgui_waterfallsink2_0.set_sample_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.osmosdr_source_0.set_sample_rate(self.samp_rate)

    def get_f_f(self):
        return self.f_f

    def set_f_f(self, f_f):
        self.f_f = f_f
        self._f_f_slider.set_value(self.f_f)
        self._f_f_text_box.set_value(self.f_f)
        self.osmosdr_source_0.set_center_freq(self.f_c + self.f_f, 0)

    def get_f_c(self):
        return self.f_c

    def set_f_c(self, f_c):
        self.f_c = f_c
        self._f_c_slider.set_value(self.f_c)
        self._f_c_text_box.set_value(self.f_c)
        self.osmosdr_source_0.set_center_freq(self.f_c + self.f_f, 0)

if __name__ == '__main__':
    import ctypes
    import os
    if os.name == 'posix':
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = top_block()
    tb.Start(True)
    tb.Wait()

