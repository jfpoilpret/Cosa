/**
 * @file Cosa/Shell.cpp
 * @version 1.0
 *
 * @section License
 * Copyright (C) 2014, Mikael Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * This file is part of the Arduino Che Cosa project.
 */

#include "Cosa/Shell.hh"

const char Shell::DEFAULT_PROMPT[] __PROGMEM = "arduino:$ ";

int
Shell::lookup(char* name) 
{
  for (uint8_t i = 0; i < m_cmdc; i++) {
    if (strcmp_P(name, (const char*) pgm_read_word(&m_cmdv[i].name)) == 0)
      return (i);
  }
  return (-1);
}

int
Shell::get(char* &option, char* &value)
{
  if (m_optind == m_argc || m_optend) return (m_optind);

  // Check for single character option and possible value
  char* arg = m_argv[m_optind];
  if (arg[0] == '-') {
    if (arg[1] == 0) {
      m_optend = false;
      return (m_optind);
    }
    arg[0] = arg[1];
    arg[1] = 0;
    value = arg + 2;
  }

  // Check for option value assignment. End of options if not found
  else {
    char* sp = strchr(arg, '=');
    if (sp == NULL) {
      m_optend = true;
      return (m_optind);
    }
    *sp = 0;
    value = sp + 1;
  }
  option = arg;
  m_optind += 1;
  return (0);
}

int
Shell::execute(char* buf)
{
  const size_t ARGV_MAX = 32;
  char* argv[ARGV_MAX];
  int argc;
  
  // Scan the line for command, options and parameters
  if (buf == NULL) return (0);
  char* bp = buf;
  char c;
  argc = 0;
  while (1) {
    c = *bp;
    // Skip white space
    while (c <= ' ' && c != 0) c = *++bp;
    if (c == 0) break;
    // Check for strings
    if (c == '"') {
      c = *++bp;
      if (c == 0) return (-1);
      argv[argc++] = bp;
      while (c != 0 && c != '"') c = *++bp;
      if (c == 0) return (-1);
    }
    // Scan token with possible string
    else {
      argv[argc++] = bp;
      while (c > ' ' && c != '"') c = *++bp;
      if (c == '"') {
	do 
	  c = *++bp; 
	while (c != 0 && c != '"');
	if (c == 0) return (-1);
	c = *++bp;
	if (c != 0 && c > ' ') return (-1);
      }
    }
    *bp++ = 0;
    if (c == 0) break;
  }
  // End the argument list and check for empty commmand line
  argv[argc] = NULL;
  m_argc = argc;
  if (argc == 0) return (0);
  
  // Lookup shell command and call action function
  int i = lookup(argv[0]);
  if (i < 0) return (-1);
  m_optind = 1;
  m_optend = false;
  m_argv = argv;
  action_fn action = (action_fn) pgm_read_word(&m_cmdv[i].action);
  const char* script = (const char*) action;
  size_t len = strlen(SHELL_SCRIPT_MAGIC);
  if (strncmp_P(SHELL_SCRIPT_MAGIC, script, len) != 0) 
    return (action(argc, argv));
  return (execute_P(script + len));
}

int 
Shell::execute_P(const char* script)
{
  char buf[BUF_MAX];
  int line;
  const char* sp = script;
  char* bp;
  int res;
  char c;
  line = 0;
  do {
    // Copy command line from program memory to buffer
    bp = buf;
    do {
      c = pgm_read_byte(sp++);
      *bp++ = c;
    } while (c != '\n' && c != 0);
    *--bp = 0;
    line += 1;
    // Execute the command and check for errors
    res = execute(buf);
    if (res != 0) return (line);
    // Continue to end of script
  } while (c != 0);
  return (0);
}

int 
Shell::run(IOStream* ins, IOStream* outs)
{
  char buf[BUF_MAX];
  if (ins == NULL) return (-1);
  if (outs != NULL) *outs << m_prompt;
  ins->get_device()->gets(buf, sizeof(buf));
  if (m_echo && outs != NULL) *outs << buf << endl;
  return (execute(buf));
}

int
Shell::help(IOStream& outs)
{
  for (uint8_t i = 0; i < m_cmdc; i++) {
    outs << (const char*) pgm_read_word(&m_cmdv[i].name) << ' ';
    outs << (const char*) pgm_read_word(&m_cmdv[i].help) << endl;
  }
  return (0);
}
