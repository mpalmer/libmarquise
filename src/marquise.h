#include <stdint.h>
#include <stdlib.h>

typedef void *marquise_consumer;
typedef void *marquise_connection;

// Attempt to start a consumer with the system's local configuration. Returns
// NULL on failure and sets errno.
//
// broker is to be specified as a zmq URI
//
// batch_period is the interval (in seconds) at which the worker thread
// will poll and empty the queue.
//
// Failure almost certainly means catastrophic failure, do not retry on
// failure, check syslog.
marquise_consumer marquise_consumer_new( char *broker, double batch_period );

// Cleanup a consumer's resources. Ensure that you run marquise_close on any
// open connections first.
void marquise_consumer_shutdown( marquise_consumer consumer );

// Open a connection to a consumer. This conection is not thread safe, the
// consumer is. If you wish to send a message from multiple threads, open a new
// connection in each.
marquise_connection marquise_connect( marquise_consumer consumer );
void marquise_close( marquise_connection connection );

// Returns 0 on success, -1 on failure, setting errno. Will only possibly fail
// on zmq_send_msg. This will probably only ever fail if you provide an invalid
// connection.

// Type: TEXT
int marquise_send_text( marquise_connection connection
                , char **source_fields
                , char **source_values
                , size_t source_count
                , char *data
                , size_t length
                // nanoseconds
                , uint64_t timestamp);

// Type: NUMBER
int marquise_send_int( marquise_connection connection
               , char **source_fields
               , char **source_values
               , size_t source_count
               , int64_t data
               // nanoseconds
               , uint64_t timestamp);

// Type: REAL
int marquise_send_real( marquise_connection connection
                , char **source_fields
                , char **source_values
                , size_t source_count
                , double data
                // nanoseconds
                , uint64_t timestamp);

// Type: EMPTY
int marquise_send_counter( marquise_connection connection
                   , char **source_fields
                   , char **source_values
                   , size_t source_count
                   // nanoseconds
                   , uint64_t timestamp);

// Type: BINARY
int marquise_send_binary( marquise_connection connection
                  , char **source_fields
                  , char **source_values
                  , size_t source_count
                  , uint8_t *data
                  , size_t length
                  // nanoseconds
                  , uint64_t timestamp);
