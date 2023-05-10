/* BOGL - Ben's Own Graphics Library.
   This file is by Edmund GRIMLEY EVANS <edmundo@rano.org>.
   Rendering design redone by Red Hat Inc, Alan Cox <alan@redhat.com>
   

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

/*
 * This provides a simple virtual terminal, with delayed refresh
 * so that it appears fast even when it isn't being fast.
 */

#define _XOPEN_SOURCE 500
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "bogl-debug.h"
#include "bogl.h"
#include "bogl-bgf.h"
#include "bogl-term.h"

static const unsigned char palette[16][3] = 
  {
    /* Colors selected by Canonical's Design Team */
    {0x01, 0x01, 0x01},	/* 0: Black. */
    {0xde, 0x38, 0x2b},	/* 1: Red. */
    {0x39, 0xb5, 0x4a},	/* 2: Green. */
    {0xff, 0xc7, 0x06},	/* 3: Brown. */
    {0x00, 0x6f, 0xb8},	/* 4: Blue. */
    {0x76, 0x26, 0x71},	/* 5: Magenta. */
    {0x2c, 0xb5, 0xe9},	/* 6: Cyan. */
    {0xe1, 0xe1, 0xe1},	/* 7: Light gray. */
    {0x80, 0x80, 0x80},	/* 0: Light Gray. */
    {0xff, 0x00, 0x00},	/* 1: Light Red. */
    {0x00, 0xff, 0x00},	/* 2: Light Green. */
    {0xff, 0xff, 0x00},	/* 3: Yellow. */
    {0x00, 0x00, 0xff},	/* 4: Light Blue. */
    {0xff, 0x00, 0xff},	/* 5: Light Magenta. */
    {0x00, 0xff, 0xff},	/* 6: Light Cyan. */
    {0xff, 0xff, 0xff},	/* 7: White. */
  };

static int child_pid = 0;
static struct termios ttysave;
static int quit = 0;

/* Out of memory.  Give up. */
static void out_of_memory (void)
{
  fprintf (stderr, "virtual memory exhausted\n");
  abort ();
}

/* Allocate AMT bytes of memory and make sure it succeeded. */
static void *xmalloc (size_t size)
{
  void *p;
  
  if (size == 0)
    return 0;
  p = malloc (size);
  if (!p)
    out_of_memory ();
  return p;
}

/* This first tries the modern Unix98 way of getting a pty, followed by the
 * old-fashioned BSD way in case that fails. */
int get_ptytty(int *xptyfd, int *xttyfd)
{
  char buf[16];
  int i, ptyfd, ttyfd;

  ptyfd = open("/dev/ptmx", O_RDWR);
  if (ptyfd >= 0) {
    const char *slave = ptsname(ptyfd);
    if (slave) {
      if (grantpt(ptyfd) >= 0) {
        if (unlockpt(ptyfd) >= 0) {
          ttyfd = open(slave, O_RDWR);
          if (ttyfd >= 0) {
            *xptyfd = ptyfd, *xttyfd = ttyfd;
            return 0;
          }
        }
      }
    }
    close(ptyfd);
  }

  for (i = 0; i < 32; i++) {
    sprintf(buf, "/dev/pty%c%x", "pqrs"[i/16], i%16);
    ptyfd = open(buf, O_RDWR);
    if (ptyfd < 0) {
      sprintf(buf, "/dev/pty/m%d", i);
      ptyfd = open(buf, O_RDWR);
    }
    if (ptyfd >= 0) {
      sprintf(buf, "/dev/tty%c%x", "pqrs"[i/16], i%16);
      ttyfd = open(buf, O_RDWR);
      if (ttyfd < 0) {
        sprintf(buf, "/dev/pty/s%d", i);
        ttyfd = open(buf, O_RDWR);
      }
      if (ttyfd >= 0) {
	*xptyfd = ptyfd, *xttyfd = ttyfd;
	return 0;
      }
      close(ptyfd);
      return 1;
    }
  }
  return 1;
}

/* Probably I should use this as a signal handler */
void send_hangup(void)
{
  if (child_pid)
    kill(child_pid, SIGHUP);
}

void sigchld(int sig)
{
  int status;
  if (waitpid(child_pid, &status, WNOHANG) > 0) {
    child_pid = 0;
    /* Reset ownership and permissions of ttyfd device? */
    tcsetattr(0, TCSAFLUSH, &ttysave);
    if (WIFEXITED (status))
      exit(WEXITSTATUS (status));
    if (WIFSIGNALED (status))
      exit(128 + WTERMSIG (status));
    if (WIFSTOPPED (status))
      exit(128 + WSTOPSIG (status));
    exit(status);
  }
  signal(SIGCHLD, sigchld);
}

void sigterm(int sig)
{
	quit = 1;
}

void spawn_shell(int ptyfd, int ttyfd, char * const *command_args)
{
  fflush(stdout);
  child_pid = fork();
  if (child_pid) {
    /* Change ownership and permissions of ttyfd device! */
    signal(SIGCHLD, sigchld);
    return;
  }

  setenv("TERM", "bterm", 1);

  close(ptyfd);

  setsid();
  ioctl(ttyfd, TIOCSCTTY, (char *)0);
  dup2(ttyfd, 0);
  dup2(ttyfd, 1);
  dup2(ttyfd, 2);
  if (ttyfd > 2)
    close(ttyfd);
  tcsetattr(0, TCSANOW, &ttysave);
  setgid(getgid());
  setuid(getuid());

  execvp(command_args[0], command_args);
  exit(127);
}

void set_window_size(int ttyfd, int x, int y)
{
  struct winsize win;

  win.ws_row = y;
  win.ws_col = x;
  win.ws_xpixel = 0;
  win.ws_xpixel = 0;
  ioctl(ttyfd, TIOCSWINSZ, &win);
}

static char *font_name;
static struct bogl_term *term;

void reload_font(int sig)
{
  struct bogl_font *font;

  font = bogl_mmap_font (font_name);
  if (font == NULL)
    {
      fprintf(stderr, "Bad font\n");
      return;
    }
  
  /* This leaks a mmap.  Since the font reloading feature is only expected
     to be used once per session (for instance, in debian-installer, after
     the font is replaced with a larger version containing more characters),
     we don't worry about the leak.  */
  free(term->font);

  term->font = font;
  term->xstep = bogl_font_glyph(term->font, ' ', 0);
  term->ystep = bogl_font_height(term->font);
}

/*
 * The usage is very simple:
 *    bterm -f font.bgf [ -l locale ] [ program ]
 */

int main(int argc, char *argv[])
{
  struct termios ntio;
  int ret;
  char buf[8192];
  struct timeval tv;
  int ptyfd, ttyfd;
  struct bogl_font *font;
  char *locale = "", *command = NULL;
  char **command_args;
  int i;
  char o = ' ';
  int pending = 0;

  for (i = 1 ; i < argc ; ++i) {
      int done = 0;
      if (argv[i][0] == '-')
          switch (argv[i][1])
          {
              case 'f':
              case 'l':
                  o = argv[i][1];
                  break;

              case '-':
                  done = 1;
                  break;

              default:
                  printf ("unknown option: %c\n", argv[i][1]);
          }
        else
            switch (o)
            {
                case ' ':
                    command = argv[i];
                    break;

                case 'f':
                    font_name = argv[i];
                    o = ' ';
                    break;

                case 'l':
                    locale = argv[i];
                    o = ' ';
                    break;
            }

      if (done)
          break;
  }

  setlocale(LC_CTYPE, locale);

  if (font_name == NULL) {
    fprintf(stderr, "Usage: %s -f font.bgf [ -l locale ] [ program ]\n", argv[0]);
 
    return 1;
  }

  if ((font = bogl_mmap_font(font_name)) == NULL) {
    fprintf(stderr, "Bad font\n");
    return 1;
  }

  tcgetattr(0, &ttysave);

  if (!bogl_init()) {
    fprintf(stderr, "bogl: %s\n", bogl_error());
    return 1;
  }

  term = bogl_term_new(font);
  if (!term)
    exit(1);

  bogl_set_palette(0, 16, palette);

  bogl_term_redraw(term);

  if (get_ptytty(&ptyfd, &ttyfd)) {
    perror("can't get a pty");
    exit(1);
  }

  if (command) {
    command_args = xmalloc(2 * sizeof *command_args);
    command_args[0] = command;
    command_args[1] = NULL;
  } else if (i < argc - 1) {
    int j;
    command_args = xmalloc((argc - i) * sizeof *command_args);
    for (j = i + 1; j < argc; ++j)
      command_args[j - (i + 1)] = argv[j];
    command_args[argc - (i + 1)] = NULL;
  } else {
    command_args = xmalloc(2 * sizeof *command_args);
    command_args[0] = "/bin/sh";
    command_args[1] = NULL;
  }
  spawn_shell(ptyfd, ttyfd, command_args);

  signal(SIGHUP, reload_font);
  signal(SIGTERM, sigterm);

  ntio = ttysave;
  ntio.c_lflag &= ~(ECHO|ISIG|ICANON|XCASE);
  ntio.c_iflag = 0;
  ntio.c_oflag &= ~OPOST;
  ntio.c_cc[VMIN] = 1;
  ntio.c_cc[VTIME] = 0;
  ntio.c_cflag |= CS8;
  ntio.c_line = 0;
  tcsetattr(0, TCSAFLUSH, &ntio);

  set_window_size(ttyfd, term->xsize, term->ysize);

  for (;;) {
    fd_set fds;
    int max = 0;

    if (quit)
	    break;
    
    if(pending)
    {
    	tv.tv_sec = 0;
    	tv.tv_usec = 0;
    }
    else
    {
    	tv.tv_sec = 10;
    	tv.tv_usec = 100000;
    }
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(ptyfd, &fds);
    if (ptyfd > max)
      max = ptyfd;
    ret = select(max+1, &fds, NULL, NULL, &tv);

    if (quit)
	    break;

    if (bogl_refresh) {
      /* Handle VT switching.  */
      if (bogl_refresh == 2)
        {
	  bogl_term_dirty (term);
	  /* This should not be necessary, but is, due to 2.6 kernel
	     bugs in the vga16fb driver.  */
	  bogl_set_palette(0, 16, palette);
	}

      bogl_refresh = 0;
      bogl_term_redraw(term);
    }
    if (ret == 0 || (ret < 0 && errno == EINTR))
    {
      if(pending)
      {
      	pending = 0;
	bogl_term_redraw(term);
      }      	
      continue;
    }

    if (ret < 0)
      perror("select");
    if (FD_ISSET(0, &fds)) {
      ret = read(0, buf, sizeof(buf));
      if (ret > 0)
	write(ptyfd, buf, ret);
    }
    else if (FD_ISSET(ptyfd,&fds)) {
      ret = read(ptyfd, buf, sizeof(buf));
      if (ret > 0)
      {
	bogl_term_out(term, buf, ret);
	pending = 1;
      }
    }
  }

  return 0;
}
