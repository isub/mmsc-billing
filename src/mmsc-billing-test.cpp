#include "billing-auxiliary.h"
#include "mmsc-billing-cdr.h"

#include <string.h>
#include <unistd.h>

int main( int argc, char* argv[ ] )
{
  int iTimes = 0;
  if ( 2 == argc ) {
    iTimes = atoi( argv[ 1 ] );
  }
  if ( 0 == iTimes ) {
    return 22;
  }

  int iCounter = 0;
  void * pVoid;
  pVoid = billing_init( "/usr/local/etc/mmsc-modules/mmsc-billing.conf" );

  if ( NULL != pVoid ) {
  } else {
    return -1;
  }

  for ( int i = 0; i < iTimes; ++i ) {
    mmsc_billing_write_cdr( "00:00:00 01.01.1900", "From", "To", "MsgId", "VASId", "SrcIFace", "DstIFace", 123 );
    sleep( ( random() % 2 ) );
    ++iCounter;
  }

  billing_fini( pVoid );

  printf( "executed %u times\r\n", iCounter );

  return 0;
}
