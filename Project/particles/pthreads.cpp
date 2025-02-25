#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include "common.h"
#include <vector>
#include <array>

//#define OLD

using namespace std;

//
//  global variables
//
int n, n_threads;
int matrixsize;
particle_t *particles;
FILE *fsave;
pthread_barrier_t barrier;
pthread_mutex_t lock_matrix;

//
//  check that pthreads routine call was successful
//
#define P( condition ) {if( (condition) != 0 ) { printf( "\n FAILURE in %s, line %d\n", __FILE__, __LINE__ );exit( 1 );}}

// a matrix of vectors containg particles, with a empty "frame" around the matrix for edge cases
vector<vector<vector<particle_t*>>> matrix;

//
//  This is where the action happens
//
void *thread_routine( void *pthread_id )
{
    int thread_id = *(int*)pthread_id;

    int particles_per_thread = (n + n_threads - 1) / n_threads;
    int first = min(  thread_id    * particles_per_thread, n );
    int last  = min( (thread_id+1) * particles_per_thread, n );
    int x, y;
    double total_clear_time, clear_time;
    //
    //  simulate a number of time steps
    //
    for( int step = 0; step < NSTEPS; step++ )
    {   
        #ifdef OLD
        //
        //  compute forces
        //
        for( int i = first; i < last; i++ )
        {
            particles[i].ax = particles[i].ay = 0;
            for (int j = 0; j < n; j++ ){
                apply_force( particles[i], particles[j] );
            }
        }
        
        pthread_barrier_wait( &barrier );
        #endif

        #ifndef OLD

        //insert
        if(thread_id == 0){
            for(int i = 0; i < n; i++){
                x = get_index(particles[i].x, matrixsize);
                y = get_index(particles[i].y, matrixsize);
                matrix[x][y].push_back(&particles[i]);
            }
        }

        pthread_barrier_wait( &barrier );

        for(int i = first; i < last; i++){
            x = get_index(particles[i].x, matrixsize);
            y = get_index(particles[i].y, matrixsize);
            particles[i].ax = particles[i].ay = 0;
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y-1].size(); k++){
                    apply_force(particles[i], *matrix[x-1+j][y-1][k]);
                }
            }
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y].size(); k++){
                    apply_force(particles[i], *matrix[x-1+j][y][k]);
                }
            }
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y+1].size(); k++){
                    apply_force(particles[i], *matrix[x-1+j][y+1][k]);
                }
            }
        }
        pthread_barrier_wait( &barrier );

        clear_time = read_timer();

        for(int i = thread_id; i < matrix.size(); i += n_threads){
            for(int j = 0; j < matrix[i].size(); j++){
                matrix[i][j].clear();
            }
        }

        total_clear_time += read_timer() - clear_time;

        #endif

        
        //
        //  move particles
        //
        for( int i = first; i < last; i++ ){
            move( particles[i] );
        }
        
        pthread_barrier_wait( &barrier );
        
        //
        //  save if necessary
        //
        if( thread_id == 0 && fsave && (step%SAVEFREQ) == 0 )
            save( fsave, n, particles );
    }

    return NULL;
}

//
//  benchmarking program
//
int main( int argc, char **argv )
{    
    //
    //  process command line
    //
    if( find_option( argc, argv, "-h" ) >= 0 )
    {
        printf( "Options:\n" );
        printf( "-h to see this help\n" );
        printf( "-n <int> to set the number of particles\n" );
        printf( "-p <int> to set the number of threads\n" );
        printf( "-s <int> to set the number of steps\n" );
        printf( "-f <int> to set the frequency of saving particle coordinates\n" );
        printf( "-o <filename> to specify the output file name\n" );
        return 0;
    }
    
    n = read_int( argc, argv, "-n", 1000 );
    int s = read_int( argc, argv, "-s", NSTEPS );
    int f = read_int( argc, argv, "-f", SAVEFREQ );
    n_threads = read_int( argc, argv, "-p", 2 );
    char *savename = read_string( argc, argv, "-o", NULL );
    int x, y;
    
    //
    //  allocate resources
    //
    fsave = savename ? fopen( savename, "w" ) : NULL;

    particles = (particle_t*) malloc( n * sizeof(particle_t) );
    set_size( n );
    init_particles( n, particles );
    matrixsize = get_gridsize();

    matrix.resize(matrixsize+2);
    for(int i = 0; i < matrixsize+2; i++){
        matrix[i].resize(matrixsize+2);
    }

    pthread_attr_t attr;
    P( pthread_attr_init( &attr ) );
    P( pthread_barrier_init( &barrier, NULL, n_threads ) );

    int *thread_ids = (int *) malloc( n_threads * sizeof( int ) );

    for( int i = 0; i < n_threads; i++ ) 
        thread_ids[i] = i;

    pthread_t *threads = (pthread_t *) malloc( n_threads * sizeof( pthread_t ) );
    
    //
    //  do the parallel work
    //
    double simulation_time = read_timer( );
    for( int i = 1; i < n_threads; i++ ) 
        P( pthread_create( &threads[i], &attr, thread_routine, &thread_ids[i] ) );
    
    thread_routine( &thread_ids[0] );
    
    for( int i = 1; i < n_threads; i++ ) 
        P( pthread_join( threads[i], NULL ) );
    simulation_time = read_timer( ) - simulation_time;
    
    printf( "n = %d, n_threads = %d, simulation time = %g seconds\n", n, n_threads, simulation_time );
    //printf("count %d\n", get_count());
    
    //
    //  release resources
    //
    P( pthread_barrier_destroy( &barrier ) );
    P( pthread_attr_destroy( &attr ) );
    free( thread_ids );
    free( threads );
    free( particles );
    if( fsave )
        fclose( fsave );
    
    return 0;
}
