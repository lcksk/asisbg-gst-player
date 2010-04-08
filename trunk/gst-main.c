/*
 * main.c
 *
 *  Created on: Mar 7, 2010
 *      Author: xpucmo
 */

#include <stdio.h>
#include <gst/gst.h>
#include <glib.h>

#include "debug.h"

typedef struct {
	GstElement * PipeLine;
	GMainLoop * loop;
	volatile gboolean play;
} xGstContainer;

static void print_tag(const GstTagList * list, const gchar * tag, gpointer unused)
{
  gint i, count;

  count = gst_tag_list_get_tag_size (list, tag);

  for (i = 0; i < count; i++) {
    gchar *str;

    if (gst_tag_get_type (tag) == G_TYPE_STRING) {
      if (!gst_tag_list_get_string_index (list, tag, i, &str))
        g_assert_not_reached ();
    } else if (gst_tag_get_type (tag) == GST_TYPE_BUFFER) {
      GstBuffer *img;

      img = gst_value_get_buffer (gst_tag_list_get_value_index (list, tag, i));
      if (img) {
        gchar *caps_str;

        caps_str = GST_BUFFER_CAPS (img) ?
            gst_caps_to_string (GST_BUFFER_CAPS (img)) : g_strdup ("unknown");
        str = g_strdup_printf ("buffer of %u bytes, type: %s",
            GST_BUFFER_SIZE (img), caps_str);
        g_free (caps_str);
      } else {
        str = g_strdup ("NULL buffer");
      }
    } else {
      str =
          g_strdup_value_contents (gst_tag_list_get_value_index (list, tag, i));
    }

    if (i == 0) {
      g_print ("%16s: %s\n", gst_tag_get_nick (tag), str);
    } else {
      g_print ("%16s: %s\n", "", str);
    }

    g_free (str);
  }
}

static int bus_call(GstBus * bus, GstMessage * msg, void * data)
{
	xGstContainer * xGstInfo = (xGstContainer *) data;
	GMainLoop * loop = xGstInfo->loop;
	gboolean buffering = FALSE;

	// g_print(">>> BUS CALL !!! <<<\n");

	switch(GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS:
		g_print("End of stream\n");
		xGstInfo->play = FALSE;
		if(loop)
			g_main_loop_quit(loop);
		break;
	case GST_MESSAGE_ERROR:
	{
		char * debug;
		GError * error;

		gst_message_parse_error(msg, &error, &debug);
		free(debug);

		g_printerr("Error %s\n", error->message);
		g_error_free(error);

		xGstInfo->play = FALSE;
		if(loop)
			g_main_loop_quit(loop);
		break;
	}
	case GST_MESSAGE_NEW_CLOCK:
	{
		GstClock *clock;

		gst_message_parse_new_clock(msg, &clock);

		g_print("New clock: %s\n", (clock ? GST_OBJECT_NAME(clock) : "NULL"));
		break;
	}
	case GST_MESSAGE_CLOCK_LOST:
		/* disabled for now as it caused problems with rtspsrc. We need to fix
		 * rtspsrc first, then release -good before we can reenable this again
		 */
		g_print("Clock lost, selecting a new one\n");
		gst_element_set_state(xGstInfo->PipeLine, GST_STATE_PAUSED);
		gst_element_set_state(xGstInfo->PipeLine, GST_STATE_PLAYING);
		break;
	case GST_MESSAGE_ELEMENT:
		g_print("ELEMENT MESSAGE\n");
		break;
	case GST_MESSAGE_TAG:
	{
		GstTagList *tags;

		if (GST_IS_ELEMENT (GST_MESSAGE_SRC (msg)))
		{
			g_print("FOUND TAG      : found by element \"%s\".\n", GST_MESSAGE_SRC_NAME (msg));
		}
		else if (GST_IS_PAD (GST_MESSAGE_SRC (msg)))
		{
			g_print("FOUND TAG      : found by pad \"%s:%s\".\n", GST_DEBUG_PAD_NAME (GST_MESSAGE_SRC (msg)));
		}
		else if (GST_IS_OBJECT (GST_MESSAGE_SRC (msg)))
		{
			g_print("FOUND TAG      : found by object \"%s\".\n", GST_MESSAGE_SRC_NAME (msg));
		}
		else
		{
			g_print("FOUND TAG\n");
		}

		gst_message_parse_tag(msg, &tags);
		gst_tag_list_foreach(tags, print_tag, NULL);
		gst_tag_list_free(tags);
	}
		break;
	case GST_MESSAGE_INFO:
	{
		GError *gerror;
		gchar *debug;
		gchar *name = gst_object_get_path_string(GST_MESSAGE_SRC (msg));

		gst_message_parse_info(msg, &gerror, &debug);
		if (debug)
		{
			g_print("INFO:\n%s\n", debug);
		}
		g_error_free(gerror);
		g_free(debug);
		g_free(name);
		break;
	}
	case GST_MESSAGE_WARNING:
	{
		GError *gerror;
		gchar *debug;
		gchar *name = gst_object_get_path_string(GST_MESSAGE_SRC (msg));

		/* dump graph on warning */
		GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (xGstInfo->PipeLine),
				GST_DEBUG_GRAPH_SHOW_ALL, "gst-launch.warning");

		gst_message_parse_warning(msg, &gerror, &debug);
		g_print("WARNING: from element %s: %s\n", name, gerror->message);
		if (debug)
		{
			g_print("Additional debug info:\n%s\n", debug);
		}
		g_error_free(gerror);
		g_free(debug);
		g_free(name);
		break;
	}
	case GST_MESSAGE_STATE_CHANGED:
	{
		GstState old, new, pending;

		gst_message_parse_state_changed(msg, &old, &new, &pending);

		/* we only care about pipeline state change messages */
		if (GST_MESSAGE_SRC (msg) != GST_OBJECT_CAST (xGstInfo->PipeLine))
			break;

		/* dump graph for pipeline state changes */
		{
			gchar *dump_name = g_strdup_printf("gst-launch.%s_%s", gst_element_state_get_name(old), gst_element_state_get_name(new));
			GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (xGstInfo->PipeLine),
					GST_DEBUG_GRAPH_SHOW_ALL, dump_name);
			g_free(dump_name);
		}

		/* ignore when we are buffering since then we mess with the states
		 * ourselves. */
		if (buffering)
		{
			g_print("Prerolled, waiting for buffering to finish...\n");
			break;
		}
		/* else not an interesting message */
		break;
	}
	case GST_MESSAGE_BUFFERING:
	{
		gint percent;

		gst_message_parse_buffering(msg, &percent);
		g_print("%s %d%%  \r", "buffering...", percent);
		break;
	}
	case GST_MESSAGE_LATENCY:
	{
		g_print("Redistribute latency...\n");
		gst_bin_recalculate_latency(GST_BIN (xGstInfo->PipeLine));
		break;
	}
	case GST_MESSAGE_REQUEST_STATE:
	{
		GstState state;
		gchar *name = gst_object_get_path_string(GST_MESSAGE_SRC (msg));

		gst_message_parse_request_state(msg, &state);

		g_print("Setting state to %s as requested by %s...\n", gst_element_state_get_name(state), name);

		gst_element_set_state(xGstInfo->PipeLine, state);

		g_free(name);
		break;
	}
	case GST_MESSAGE_APPLICATION:
	{
		const GstStructure *s;

		s = gst_message_get_structure(msg);

		if (gst_structure_has_name(s, "GstLaunchInterrupt"))
		{
			/* this application message is posted when we caught an interrupt and
			 * we need to stop the pipeline. */

			g_print("Interrupt: Stopping pipeline ...\n");
		}
	}
	case GST_MESSAGE_STREAM_STATUS: {
		GstStreamStatusType stype;
		GstElement * owner;
		gst_message_parse_stream_status(msg, &stype, &owner);
		g_print("Stream status: %d\n", stype);
	}
		break;
	case GST_MESSAGE_ASYNC_DONE:
		g_print("Async done.\n");
		break;
	default:
		g_print("GST MESSAGE: %d\n", GST_MESSAGE_TYPE(msg));
		break;
	}

	return TRUE;
}

static void on_pad_added(GstElement * element, GstPad * pad, void * data)
{
	GstPad * sinkpad;
	GstElement * decoder = (GstElement *) data;

	/* We can now link this pad with the vorbis-decoder sink pad */
	g_print("Dynamic pad created, linking demuxer/decoder\n");
	sinkpad = gst_element_get_static_pad (decoder, "sink");
	gst_pad_link (pad, sinkpad);
	gst_object_unref(sinkpad);
}

static inline void add_static_ghost_pad(GstElement * bin, GstElement * el, char * name)
{
	GstPad * pad;

	pad = gst_element_get_static_pad(el, name);
	gst_element_add_pad(bin, gst_ghost_pad_new(name, pad));
	gst_object_unref(GST_OBJECT(pad));
}

static gboolean print_position(GstElement *pipeline)
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len;
	if (gst_element_query_position(pipeline, &fmt, &pos) && gst_element_query_duration(pipeline, &fmt, &len)) {
		g_print("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r", GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
	}
	/* call me again */
	return TRUE;
}

static void seek_to_time(GstElement *pipeline, gint64 time_nanoseconds)
{
	if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
		g_print("Seek failed!\n");
	}
}

#define XOR(a, b)	(((a) && (b)) || (!(a) && !(b)))
#define SCR_W	800
#define SCR_H	480

enum MfwGstVpuDecCodecs {
	std_mpeg4,
	std_h263,
	std_avc,
};

#define VIDEO_QUEUE	1

GstElement * getVideoPlayBin(enum MfwGstVpuDecCodecs codec)
{
	GstElement * VideoBin = NULL;
#ifdef VIDEO_QUEUE
	GstElement * VideoQueue0 = gst_element_factory_make("queue", "video_queue0");
#endif
	// GstElement * VideoQueue1 = gst_element_factory_make("queue", "video_queue1");
#ifdef MACH_IMX27
	GstElement * VideoDec = gst_element_factory_make("mfw_vpudecoder", "video_decoder"); // ffdec_mpeg4
	GstElement * VideoSink = gst_element_factory_make("mfw_v4lsink", "video_sink"); // xvimagesink
#else
	GstElement * VideoDec = gst_element_factory_make("ffdec_mpeg4", "video_decoder");
	GstElement * VideoSink = gst_element_factory_make("xvimagesink", "video_sink");
#endif
	// GstElement * VideoConv = gst_element_factory_make("textoverlay", "video_convertor");

#ifdef VIDEO_QUEUE
	if(VideoQueue0 && VideoDec && VideoSink) {
#else
	if(VideoDec && VideoSink) {
#endif
		VideoBin = gst_bin_new("video_bin");
#ifdef VIDEO_QUEUE
		g_object_set(G_OBJECT(VideoQueue0), "max-size-buffers", 2, NULL);
		g_object_set(G_OBJECT(VideoQueue0), "max-size-time", 400, NULL);
		g_object_set(G_OBJECT(VideoQueue0), "max-size-bytes", 400, NULL);
#endif
#ifdef MACH_IMX27
		g_object_set(G_OBJECT(VideoDec), "codec-type", codec, NULL);
		g_object_set(G_OBJECT(VideoSink), "disp-width", SCR_W, "disp-height", SCR_H, NULL);
		g_object_set(G_OBJECT(VideoSink), "sync", FALSE, NULL);
#else
		g_object_set(G_OBJECT(VideoSink), "max-lateness", 10, NULL);
		g_object_set(G_OBJECT(VideoSink), "async", TRUE, NULL);
#endif
		// g_object_set(G_OBJECT(VideoConv), "text", "Asis-BG", NULL);
#ifdef VIDEO_QUEUE
		gst_bin_add_many(GST_BIN(VideoBin), VideoQueue0, VideoDec,/* VideoConv,*/ VideoSink, NULL);
		gst_element_link_many(VideoQueue0, VideoDec,/* VideoConv,*/ VideoSink, NULL);
		add_static_ghost_pad(VideoBin, VideoQueue0, "sink");
#else
		gst_bin_add_many(GST_BIN(VideoBin), VideoDec,/* VideoConv,*/ VideoSink, NULL);
		gst_element_link_many(VideoDec,/* VideoConv,*/ VideoSink, NULL);
		add_static_ghost_pad(VideoBin, VideoDec, "sink");
#endif
	}

	return VideoBin;
}

GstElement * getAudioPlayBin(gchar * decoder)
{
	GstElement * AudioBin = NULL;
	GstElement * AudioQueue0 = gst_element_factory_make("queue", "audio_queue0");
	// GstElement * AudioQueue1 = gst_element_factory_make("queue", "audio_queue1");
	GstElement * AudioSink = gst_element_factory_make("alsasink", "audio_sink");
	GstElement * AudioDec = NULL;

	if(decoder && g_strcmp0(decoder, "pcm")) {
		AudioDec = gst_element_factory_make(decoder, "audio_decoder");
	}

	if(AudioQueue0 && XOR((decoder && g_strcmp0(decoder, "pcm")), AudioDec) && AudioSink) {
		AudioBin = gst_bin_new("audio_bin");

		// g_object_set(G_OBJECT(AudioQueue0), "max-size-buffers", 2, NULL);
		// g_object_set(G_OBJECT(AudioQueue0), "max-size-time", 0, NULL);
		// g_object_set(G_OBJECT(AudioQueue0), "max-size-bytes", 0, NULL);

		g_object_set(G_OBJECT(AudioSink), "sync", FALSE, NULL);

		// gst_bin_add_many(GST_BIN(AudioBin), AudioQueue0,/* AudioDec,*/ AudioSink, NULL);
		// gst_element_link_many(AudioQueue0,/* AudioDec,*/ AudioSink, NULL);

		gst_bin_add_many(GST_BIN(AudioBin), AudioQueue0, AudioSink, NULL);

		if(AudioDec) {
			gst_bin_add(GST_BIN(AudioBin), AudioDec);
			gst_element_link_many(AudioQueue0, AudioDec, AudioSink, NULL);
		}
		else {
			gst_element_link_many(AudioQueue0, AudioSink, NULL);
		}
		add_static_ghost_pad(AudioBin, AudioQueue0, "sink");
	}

	return AudioBin;
}

GstElement * initPipeLine(gchar * name, int vcodec, gchar * acodec)
{
	gchar * Name;
	GstElement * PipeLine = NULL;
	GstElement * Source = NULL;
	GstElement * Demuxer = NULL;
	GstElement * VideoBin = NULL;
	GstElement * AudioBin = NULL;

	if(!(name && ((vcodec >= 0) || acodec))) {
		Name = NULL;
		PipeLine = NULL;
		return NULL;
	}

	Name = g_strdup(name);

	if(!g_file_test(Name, G_FILE_TEST_EXISTS)) {
		g_free(Name);
		Name = NULL;
		return NULL;
	}

	if(vcodec >= 0)
		VideoBin = getVideoPlayBin((enum MfwGstVpuDecCodecs)std_mpeg4);

	if(acodec)
		AudioBin = getAudioPlayBin(acodec);

	PipeLine = gst_pipeline_new("pipeline");
	Source = gst_element_factory_make("filesrc", "source");
	Demuxer = gst_element_factory_make("avidemux", "avi_demuxer");

	if(!(Source && Demuxer && (VideoBin || AudioBin))) {
		g_free(Name);
		Name = NULL;
		gst_object_unref(GST_OBJECT(Source));
		gst_object_unref(GST_OBJECT(Demuxer));
		gst_object_unref(GST_OBJECT(VideoBin));
		gst_object_unref(GST_OBJECT(AudioBin));
		return NULL;
	}

	g_object_set(G_OBJECT(Source), "location", Name, NULL);
	g_object_set(G_OBJECT(Source), "use-mmap", TRUE, NULL);
	g_object_set(G_OBJECT(Source), "typefind", TRUE, NULL);
	g_object_set(G_OBJECT(Source), "touch", TRUE, NULL);
	g_object_set(G_OBJECT(Source), "blocksize", 1600000, NULL);

	gst_bin_add_many(GST_BIN(PipeLine), Source, Demuxer, NULL);
	gst_element_link(Source, Demuxer);

	if(VideoBin) {
		gst_bin_add(GST_BIN(PipeLine), VideoBin);
		g_signal_connect(Demuxer, "pad-added", G_CALLBACK(on_pad_added), VideoBin);
	}

	if(AudioBin) {
		gst_bin_add(GST_BIN(PipeLine), AudioBin);
		g_signal_connect(Demuxer, "pad-added", G_CALLBACK(on_pad_added), AudioBin);
	}

	return PipeLine;
}

void freePipeLine(GstElement * PipeLine)
{
	if(PipeLine) {
		gst_object_unref(GST_OBJECT(PipeLine));
		PipeLine = NULL;
	}
}

int main (int argc, char *argv[])
{
	xGstContainer * xGstInfo = g_malloc(sizeof(xGstContainer));
	GstBus * bus;
	GstStateChangeReturn stret;
	// gint LoopCounter = 0;
	GstState state, pending;

	if(argc < 2) {
		g_printerr("Usage: %s <filename>\n", argv[0]);
		return -1;
	}

	gst_init (&argc, &argv);

	xGstInfo->loop = NULL; // g_main_loop_new (NULL, FALSE);

	// create pipeline
	//xGstInfo->PipeLine = initPipeLine(argv[1], std_mpeg4, "pcm");
	xGstInfo->PipeLine = initPipeLine(argv[1], std_mpeg4, NULL);

	if(!xGstInfo->PipeLine) {
		g_printerr("Pipeline not created.\n");
		return -1;
	}

	// add message handler
	bus = gst_pipeline_get_bus(GST_PIPELINE(xGstInfo->PipeLine));
	if(xGstInfo->loop) {
		gst_bus_add_watch(bus, bus_call, xGstInfo);
		gst_object_unref(bus);
	}

	g_print("Now playing %s\n", argv[1]);

	// set state
	xGstInfo->play = TRUE;
	stret = gst_element_set_state(xGstInfo->PipeLine, GST_STATE_NULL); // GST_STATE_NULL, GST_STATE_READY, GST_STATE_PAUSED, GST_STATE_PLAYING
	g_print("state change result: %d\n", stret);
	if(stret == GST_STATE_CHANGE_FAILURE)
		return -1;
	stret = gst_element_set_state(xGstInfo->PipeLine, GST_STATE_PLAYING); // GST_STATE_NULL, GST_STATE_READY, GST_STATE_PAUSED, GST_STATE_PLAYING
	g_print("state change result: %d\n", stret);
	if(stret == GST_STATE_CHANGE_FAILURE)
		return -1;

	// seek_to_time(xGstInfo->PipeLine, 10000000);

	// iterate
	g_print("Running...\n");

#ifndef MACH_IMX27
	g_timeout_add(1000, (GSourceFunc) print_position, xGstInfo->PipeLine);
#endif

	if(xGstInfo->loop) {
		g_main_loop_run(xGstInfo->loop);
	}
	else {
		GstMessage * message;
		while(xGstInfo->play) {
			if((message = gst_bus_poll(bus, GST_MESSAGE_ANY, -1)) != NULL) {
				bus_call(bus, message, xGstInfo);
			}
		}
	}

	// gst_element_query()
	// gst_element_query_position()
	// gst_element_query_duration()

	while (g_main_context_iteration (NULL, FALSE));

	if(xGstInfo->loop) {
		g_main_loop_quit(xGstInfo->loop);
		g_main_loop_unref(xGstInfo->loop);
	}
	else {
		gst_object_unref(bus);
	}

	// Out of the main loop, clean up nicely
	g_print("Returned, setting paused...\n");
	stret = gst_element_set_state (xGstInfo->PipeLine, GST_STATE_READY);
	gst_element_get_state (xGstInfo->PipeLine, &state, &pending, GST_CLOCK_TIME_NONE);
	g_print("Returned, setting ready...\n");
	stret = gst_element_set_state (xGstInfo->PipeLine, GST_STATE_READY);
	gst_element_get_state (xGstInfo->PipeLine, &state, &pending, GST_CLOCK_TIME_NONE);
	g_print("stopping playback\n");
#ifndef MACH_IMX27
	stret = gst_element_set_state (xGstInfo->PipeLine, GST_STATE_NULL);
	gst_element_get_state (xGstInfo->PipeLine, &state, &pending, GST_CLOCK_TIME_NONE);
#endif
	g_print("state change result: %d\n", stret);

	//unref pipeline
	g_print("Deleting pipeline\n");
	freePipeLine(xGstInfo->PipeLine);

	g_free(xGstInfo);

	g_print("Exit\n");

	return 0;
}
