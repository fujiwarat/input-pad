%module input_pad_group

%{
#include <input-pad-group.h>
%}

%include input-pad-group.h

%pythoncode
{
def parse_all_files(paddir=None):
    return input_pad_group_parse_all_files (paddir)
}
