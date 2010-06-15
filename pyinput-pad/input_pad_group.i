%module input_pad_group

%{
#include <input-pad-group.h>
%}

%include input-pad-group.h

%pythoncode
{
def parse_all_files(custom_dirname=None, domain=None):
    return input_pad_group_parse_all_files (custom_dirname, domain)
}
