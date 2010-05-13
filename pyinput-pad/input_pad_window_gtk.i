%module input_pad_window_gtk

%{
#include <input-pad-window-gtk-swig.h>
InputPadGtkWindow *
_input_pad_gtk_window_new_wrapper (int pytype, unsigned int ibus);
unsigned long
_input_pad_gtk_window_connect_wrapper (struct _InputPadGtkWindow *self,
                                       char *signal_id,
                                       PyObject *pysignal_cb,
                                       PyObject *pydata);
%}

%include input-pad-window-gtk-swig.h
%extend _InputPadGtkWindow {
    _InputPadGtkWindow (int pytype,
                        unsigned int ibus) {
        return _input_pad_gtk_window_new_wrapper (pytype, ibus);
    }
    void hide () {
        gtk_widget_hide (GTK_WIDGET (self));
    }
    void show () {
        gtk_widget_show (GTK_WIDGET (self));
    }
    void show_all () {
        gtk_widget_show_all (GTK_WIDGET (self));
    }
    void connect (char *signal_id, PyObject *signal_cb,
                  PyObject *data=Py_None) {
        _input_pad_gtk_window_connect_wrapper (self, signal_id,
                                               signal_cb, data);
    }
    ~_InputPadGtkWindow () {
        /* We don't need anything? */
    }
};

%inline %{
/* workaround */
extern GtkWidget *
_input_pad_gtk_window_new_with_gtype (GtkWindowType type,
                                      unsigned int child,
                                      gboolean     gtype);

InputPadGtkWindow *
_input_pad_gtk_window_new_wrapper (int pytype, unsigned int ibus)
{
    GtkWindowType type;
    type = pytype;
    return (InputPadGtkWindow *)
        _input_pad_gtk_window_new_with_gtype (type, ibus, TRUE);
}

typedef struct _python_callback_data
{
    PyObject *pysignal_cb;
    PyObject *pydata;
} python_callback_data;

static unsigned int
button_pressed_cb (struct _InputPadGtkWindow *window,
                   gchar *str,
                   guint type,
                   guint keysym,
                   guint keycode,
                   guint state,
                   gpointer data)
{
    PyObject *retval;
    PyObject *pywindow = NULL;
    PyObject *pystr = NULL;
    python_callback_data *cb_data = (python_callback_data *) data;

    pywindow  = SWIG_NewPointerObj (SWIG_as_voidptr(window),
                                    SWIGTYPE_p__InputPadGtkWindow, 0 |  0 );
    pystr = PyString_FromString (str);
    retval = PyObject_CallFunction (cb_data->pysignal_cb,
                                   "OOiiiiO",
                                   pywindow, pystr, type,
                                   keysym, keycode, state,
                                   cb_data->pydata);
    Py_DECREF (pystr);

    return FALSE;
}

unsigned long
_input_pad_gtk_window_connect_wrapper (struct _InputPadGtkWindow *self,
                                       char *signal_id,
                                       PyObject *pysignal_cb,
                                       PyObject *pydata)
{
    gchar *message = NULL;
    python_callback_data *cb_data;

    if (!PyCallable_Check (pysignal_cb)) {
        PyErr_Warn (PyExc_Warning, "not function");
        return 0;
    }

    if (g_strcmp0 (signal_id, "button-pressed") != 0) {
        message = g_strdup_printf ("Your signal_id is not supported: %s",
                                   signal_id ? signal_id : "(null)");
        PyErr_Warn (PyExc_Warning, message);
        g_free (message);
        return 0;
    }

    cb_data = (python_callback_data *) g_new0 (python_callback_data, 1);
    cb_data->pysignal_cb = pysignal_cb;
    cb_data->pydata = pydata;
    return g_signal_connect (G_OBJECT (self), signal_id,
                             G_CALLBACK (button_pressed_cb), cb_data);
}
%}
