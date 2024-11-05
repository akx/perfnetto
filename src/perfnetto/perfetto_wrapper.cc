// TODO: should use .h here and compile the .cc separately
#include "../../vendor/perfetto/sdk/perfetto.cc"

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("PythonFunc"),
    perfetto::Category("PythonMark"),
    perfetto::Category("PythonRegion"),
);
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

std::unique_ptr<perfetto::TracingSession> tracing_session;
int perfnetto_initialized = 0;

static const char *get_frame_function_name(PyFrameObject *frame) {
  // TODO: this should also get the module name, etc. when possible, and do it
  // fast
  if (!frame) {
    return "<invalid frame>";
  }

  PyCodeObject *code = PyFrame_GetCode(frame);
  if (!code) {
    return "<no code object>";
  }

  PyObject *name = code->co_name;
  if (!name || !PyUnicode_Check(name)) {
    Py_DECREF(name);
    Py_DECREF(code);
    return "<invalid name>";
  }

  const char *name_str = PyUnicode_AsUTF8(name);
  Py_DECREF(name);
  Py_DECREF(code);
  return name_str ? name_str : "<encoding error>";
}

// TODO: this should probably use the sys.monitoring API added in 3.12 when
//       available
int pw_tracefunc(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg) {
  if (what == PyTrace_CALL || what == PyTrace_C_CALL) {
    TRACE_EVENT_BEGIN("PythonFunc", nullptr, [&](perfetto::EventContext ctx) {
      ctx.event()->set_name(get_frame_function_name(frame));
    });
  } else if (what == PyTrace_RETURN || what == PyTrace_EXCEPTION || what == PyTrace_C_RETURN || what == PyTrace_C_EXCEPTION) {
    TRACE_EVENT_END("PythonFunc");
  }
  return 0;
}

void pw_start_region(const char *name) {
  TRACE_EVENT_BEGIN("PythonRegion", nullptr, [&](perfetto::EventContext ctx) {
    ctx.event()->set_name(name);
  });
}

void pw_end_region() { TRACE_EVENT_END("PythonRegion"); }

void pw_init_perfetto(bool use_system_backend) {
  if (perfnetto_initialized) {
    return;
  }
  perfetto::TracingInitArgs args;
  args.backends |= perfetto::kInProcessBackend;
  if (use_system_backend) {
    args.backends |= perfetto::kSystemBackend;
  }
  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
  perfnetto_initialized = 1;
}

int pw_start_tracing(bool all_threads, int size_kb, const char *output_file, const char *session_name) {
  // TODO: should ensure this can't be called reentrantly etc.
  if (tracing_session.get() != nullptr) {
    return 42;
  }
  if (!perfnetto_initialized) {
    pw_init_perfetto(false);
  }

  perfetto::protos::gen::TrackEventConfig track_event_cfg;

  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(size_kb);
  if (output_file != nullptr) {
    cfg.set_write_into_file(true);
    cfg.set_output_path(output_file);
  }
  if (session_name != nullptr) {
    cfg.set_unique_session_name(session_name);
  }
  auto *ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");
  ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());
  tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
#if PY_VERSION_HEX >= 0x03120000
  if (all_threads) {
    PyEval_SetProfileAllThreads(pw_tracefunc, NULL);
    return 0;
  }
#endif
  PyEval_SetProfile(pw_tracefunc, NULL);
  return 0;
}

std::vector<char> pw_stop_tracing(bool read_data) {
  // TODO: should ensure this can't be called reentrantly etc.
  if (!tracing_session) {
    return std::vector<char>();
  }
  PyEval_SetTrace(NULL, NULL);
  auto sess = tracing_session.release();
  sess->StopBlocking();
  if (!read_data) {
    return std::vector<char>();
  }
  std::vector<char> trace_data(sess->ReadTraceBlocking());
  return trace_data;
}

void pw_instant(const char *name) {
  TRACE_EVENT_INSTANT("PythonMark", perfetto::DynamicString{name});
}
