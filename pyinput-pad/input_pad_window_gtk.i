%module input_pad_window_gtk

%{
#include <input-pad-window-gtk-swig.h>
InputPadGtkWindow *
_input_pad_gtk_window_new_wrapper (int                  pytype,
                                   unsigned int         child);
unsigned long
_input_pad_gtk_window_connect_wrapper (struct _InputPadGtkWindow *self,
                                       char *signal_id,
                                       PyObject *pysignal_cb,
                                       PyObject *pydata);
%}

%include input-pad-window-gtk-swig.h

%pythoncode
{
class InputPadGtkWindow(_InputPadGtkWindow):
    def __init__(self, pytype, child=0):
        _InputPadGtkWindow.__init__(self, pytype, child)
    def set_paddir(self, paddir, domain=None):
        _InputPadGtkWindow.set_paddir(self, paddir, domain)
    def append_padfile(self, padfile, domain=None):
        _InputPadGtkWindow.append_padfile(self, padfile, domain)

def get_kbdui_name_list():
    return _input_pad_gtk_window_get_kbdui_name_list_wrapper()
}

%extend _InputPadGtkWindow {
    _InputPadGtkWindow (int             pytype,
                        unsigned int    child) {
        return _input_pad_gtk_window_new_wrapper (pytype, child);
    }
    void hide () {
        gtk_widget_hide (GTK_WIDGET (self));
    }
    void show () {
        gtk_widget_show (GTK_WIDGET (self));
    }
    int get_visible () {
        return gtk_widget_get_visible (GTK_WIDGET (self));
    }
    void set_paddir (const char *paddir, const char *domain) {
        input_pad_gtk_window_set_paddir (INPUT_PAD_GTK_WINDOW (self),
                                         paddir, domain);
    }
    void append_padfile (const char *padfile, const char *domain) {
        input_pad_gtk_window_append_padfile (INPUT_PAD_GTK_WINDOW (self),
                                             padfile, domain);
    }
    void set_char_button_sensitive (unsigned int sensitive) {
        input_pad_gtk_window_set_char_button_sensitive (INPUT_PAD_GTK_WINDOW (self),
                                                        sensitive ? TRUE : FALSE);
    }
    void reorder_button_pressed () {
        input_pad_gtk_window_reorder_button_pressed (INPUT_PAD_GTK_WINDOW (self));
    }
    unsigned int get_keyboard_state () {
        return input_pad_gtk_window_get_keyboard_state (INPUT_PAD_GTK_WINDOW (self));
    }
    void set_keyboard_state (unsigned int keyboard_state) {
        input_pad_gtk_window_set_keyboard_state (INPUT_PAD_GTK_WINDOW (self),
                                                 keyboard_state);
    }
    void set_keyboard_state_with_keysym (unsigned int keysym) {
        input_pad_gtk_window_set_keyboard_state_with_keysym (INPUT_PAD_GTK_WINDOW (self),
                                                             keysym);
    }
    void set_kbdui_name (const char *kbdui_name) {
        input_pad_gtk_window_set_kbdui_name (INPUT_PAD_GTK_WINDOW (self),
                                             kbdui_name);
    }
    void set_show_table (unsigned int type) {
        input_pad_gtk_window_set_show_table (INPUT_PAD_GTK_WINDOW (self),
                                             type);
    }
    void set_show_layout (unsigned int type) {
        input_pad_gtk_window_set_show_layout (INPUT_PAD_GTK_WINDOW (self),
                                              type);
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
_input_pad_gtk_window_new_with_gtype (GtkWindowType     type,
                                      unsigned int      child,
                                      gboolean          gtype);

PyObject *
_input_pad_gtk_window_get_kbdui_name_list_wrapper (void)
{
    InputPadWindowKbduiName *list;
    Py_ssize_t i, size = 0;
    PyObject *retval = NULL;
    PyObject *tuple = NULL;

    list = input_pad_gtk_window_get_kbdui_name_list ();
    if (list == NULL) {
        return Py_None;
    }
    while (list[size].name != NULL) {
        size++;
    }
    retval = PyList_New (0);
    for (i = 0; i < size; i++) {
        tuple = PyTuple_Pack (3,
                              PyString_FromString (list[i].name),
                              PyString_FromString (list[i].description),
                              PyInt_FromLong (list[i].type));
        PyList_Append (retval, tuple);
        g_free (list[i].name);
        list[i].name = NULL;
        g_free (list[i].description);
        list[i].description = NULL;
    }
    g_free (list);
    return retval;
}

InputPadGtkWindow *
_input_pad_gtk_window_new_wrapper (int                  pytype,
                                   unsigned int         child)
{
    GtkWindowType type;
    type = pytype;
    return (InputPadGtkWindow *)
        _input_pad_gtk_window_new_with_gtype (type, child, TRUE);
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
