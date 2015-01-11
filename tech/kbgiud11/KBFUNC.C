/* KBFunc.c */
/* Demonstrates the use of the keyboard and defines some functions */
/* Copyright (c) 1994 Gertjan Klein */
/* Freeware. See also legal information in kbguide.txt */


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

int KeybID(void);
int ScanSet(int);
int WaitForKeyb(void);
void PrintKeys(int);

main(int argc, char **argv)
{
  int result, set;

  set = atoi(argv[1]);
  if( (argc < 2) | (set > 3) | (set < 1) )

  {
    puts("Usage: KEYBID <1..3>");
    exit(1);
  }
  result = KeybID();		      /*Determine keyboard type*/
  switch (result)
  {
  case 0 :
    puts("\nThis is an XT keyboard.");
    break;
  case 1 :
    puts("\nThis is an AT keyboard.");
    break;
  case 2 :
    puts("\nThis is an MF II keyboard.");
    break;
  default:
    puts("\nError determining keyboard.");
  }
  if( (result == 1) | (result == 2) )
  {				      /*AT or MF II keyboard: can set scanset*/
    if( ScanSet(set) )		      /*If scanset set succesfully*/
    {
      printf("Scanset set to %d\n", set);
      PrintKeys(set);		      /*Print hex codes of keys pressed*/
    }
    puts("");			      /*Newline*/
    ScanSet(2); 		      /*Normal scanset back (is 2!)*/
  }
}

int KeybID(void)
{
  int ret_code;

  outp(0x21, 0x02);		      /*Block IRQ1*/
  outp(0x60, 0xf2);		      /*Give command*/
  if( ! WaitForKeyb() )
    ret_code = 0;		      /*XT-keyboard*/
  else
  {
    if( inp(0x60) != 0xfa )
      ret_code = 4;		      /*No ACK: error*/
    else
    {
      if( ! WaitForKeyb() )
	ret_code = 1;		      /*No ID byte: normal AT keyboard*/
      else
      {
	if( inp(0x60) != 0xab)
	  ret_code = 5; 	      /*Unknown ID byte: error*/
	else
	{
	  if( ! WaitForKeyb() )
	    ret_code = 6;	      /*No second ID byte: error*/
	  else
	  {
	    if( inp(0x60) != 0x41 )
	      ret_code = 7;	      /*Unknown second ID byte: error*/
	    else
	      ret_code = 2;	      /*MF II keyboard*/
	  }
	}
      }
    }
  }
  outp(0x21,0x00);		      /*Reenable IRQ1*/
  return(ret_code);
}

int ScanSet(set)
{
  int result;

  outp(0x21, 0x02);		      /*Block IRQ1*/
  outp(0x60, 0xf0);		      /*Give command*/
  if( WaitForKeyb() )		      /*If keyboard responded*/
  {
    if( inp(0x60) == 0xfa)	      /*If response was ACK*/
    {
      outp(0x60,set);		      /*Send scanset to use*/
      if( WaitForKeyb() )	      /*If keyboard responded*/
      {
	if( inp(0x60) == 0xfa)	      /*If response was ACK*/
	  result = 1;		      /*Then scanset set properly*/
	else result = 0;	      /*Wrong ACK: error*/
      }
      else result = 0;		      /*No ACK: error*/
    }
    else result = 0;		      /*Wrong ACK: error*/
  }
  else result = 0;		      /*No ACK: error*/
  outp(0x21,0x00);		      /*Reenable IRQ1*/
  return(result);
}

int WaitForKeyb(void)
{
  unsigned i;
  int tmp;
  for(i=0; i < 0xffff; i++)	      /*Reasonable waiting loop*/
  {
    tmp = inp(0x64);		      /*Get status port*/
    if( (tmp & 0x01) == 0x01 )	      /*If char waiting in buffer*/
      return(tmp & 0x01);	      /*Go back*/
  }
  return(tmp & 0x01);		      /*No char after waiting: go back*/
}

void PrintKeys(int set)
{
  int cnt = 0, ch, esc = 0, loop = 1;

	outp(0x21,0x02);		    /*Turn off IRQ1*/
	while( (inp(0x64) & 0x01) == 0x01)  /*Kill any chars waiting for us*/
		ch = inp(0x60); 	    /*(Start clean)*/
  do				      /*Do until ESC key pressed*/
  {
    for(;;)			      /*Eternal loop*/
      if( (inp(0x64) & 0x01) == 0x01 )
	break;			      /*Unless key in buffer*/
    printf("%02X ", (ch = inp(0x60)) );   /*Print it*/
    switch (set)		      /*Check for ESC*/
    {
      case 1:			      /*Scanset 1 seq. for ESC: 0x43 0x00*/
	if( (ch == 0x43) && (esc == 0) )      /*If make code found*/
	  esc = 1;			      /*Remember*/
	else if( (ch == 0x00) && (esc == 1) ) /*If break after make found*/
	  loop = 0;		      /*User wants out*/
	else esc = 0;
	break;
      case 2:			      /*Scanset 2 seq. for ESC: 0x01 0x81 */
	if( (ch == 01) && (esc == 0) )	      /*If make code found*/
	  esc = 1;			      /*Remember*/
	else if( (ch == 0x81) && (esc == 1) ) /*If break after make found*/
	  loop = 0;		      /*User wants out*/
	else esc = 0;
	break;
      case 3:			      /*Scanset 3: 0x64 (no break) for ESC*/
	if(ch == 0x64)		      /*If ESC key pressed*/
	  loop = 0;		      /*User wants out*/
	break;
    }
    if( ++cnt == 24 )		      /*If we've placed 24 numbers on screen*/
    {
      cnt = 0;			      /*Its time for a*/
      puts(""); 		      /*New line*/
    }
  }while(loop);
  outp(0x21,0x00);		      /*Reenable IRQ1*/
}