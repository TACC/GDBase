#ifndef FINDEXEC_H
#define FINDEXEC_H

int find_exec_in_path( const char * cmd, int* plen, char ** abspath);
int find_gdbase_prefix(const char * cmd, int* sz,   char ** prefix);

#endif /* FINDEXEC_H */
