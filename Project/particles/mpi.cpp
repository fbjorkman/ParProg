#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include "common.h"

//#define OLD

using namespace std;

//
//  benchmarking program
//
int main( int argc, char **argv )
{    
    //
    //  process command line parameters
    //
    if( find_option( argc, argv, "-h" ) >= 0 )
    {
        printf( "Options:\n" );
        printf( "-h to see this help\n" );
        printf( "-n <int> to set the number of particles\n" );
        printf( "-s <int> to set the number of steps\n" );
        printf( "-f <int> to set the frequency of saving particle coordinates\n" );
        printf( "-o <filename> to specify the output file name\n" );
        return 0;
    }
    
    int n = read_int( argc, argv, "-n", 1000 );
    int s = read_int( argc, argv, "-s", NSTEPS );
    int f = read_int( argc, argv, "-f", SAVEFREQ );
    char *savename = read_string( argc, argv, "-o", NULL );

    // a matrix of vectors containg particles, with a empty "frame" around the matrix for edge cases
    vector<vector<vector<particle_t*>>> matrix;
    
    //
    //  set up MPI
    //
    int n_proc, rank;
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &n_proc );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    
    //
    //  allocate generic resources
    //
    FILE *fsave = savename && rank == 0 ? fopen( savename, "w" ) : NULL;
    particle_t *particles = (particle_t*) malloc( n * sizeof(particle_t) );
    init_particles( n, particles );
    
    MPI_Datatype PARTICLE;
    MPI_Type_contiguous( 6, MPI_DOUBLE, &PARTICLE );
    MPI_Type_commit( &PARTICLE );
    
    //
    //  set up the data partitioning across processors
    //
    int particle_per_proc = (n + n_proc - 1) / n_proc;
    int *partition_offsets = (int*) malloc( (n_proc+1) * sizeof(int) );
    for( int i = 0; i < n_proc+1; i++ )
        partition_offsets[i] = min( i * particle_per_proc, n );
    
    int *partition_sizes = (int*) malloc( n_proc * sizeof(int) );
    for( int i = 0; i < n_proc; i++ )
        partition_sizes[i] = partition_offsets[i+1] - partition_offsets[i];
    
    //
    //  allocate storage for local partition
    //
    int nlocal = partition_sizes[rank];
    particle_t *local = (particle_t*) malloc( nlocal * sizeof(particle_t) );
    
    //
    //  initialize and distribute the particles (that's fine to leave it unoptimized)
    //
    set_size( n );
    if( rank == 0 )
        init_particles( n, particles );
    MPI_Scatterv( particles, partition_sizes, partition_offsets, PARTICLE, local, nlocal, PARTICLE, 0, MPI_COMM_WORLD );

    #ifndef OLD
    int matrixsize = get_gridsize();
    matrix.resize(matrixsize+2);
    for(int i = 0; i < matrixsize+2; i++){
        matrix[i].resize(matrixsize+2);
    }

    #endif

    //
    //  simulate a number of time steps
    //
    double simulation_time = read_timer( );
    for( int step = 0; step < s; step++ )
    {     
        // 
        //  collect all global data locally (not good idea to do)
        //
        MPI_Allgatherv( local, nlocal, PARTICLE, particles, partition_sizes, partition_offsets, PARTICLE, MPI_COMM_WORLD );
        //
        //  save current step if necessary (slightly different semantics than in other codes)
        //
        if( fsave && (step%f) == 0 )
            save( fsave, n, particles );
        
        #ifdef OLD
        //
        //  compute all forces
        //
        for( int i = 0; i < nlocal; i++ )
        {
            local[i].ax = local[i].ay = 0;
            for (int j = 0; j < n; j++ )
                apply_force( local[i], particles[j] );
        }
        #endif
        #ifndef OLD
        int x, y;
        for(int i = 0; i < n; i++){
            x = get_index(particles[i].x, matrixsize);
            y = get_index(particles[i].y, matrixsize);
            matrix[x][y].push_back(&particles[i]);
        }
        
        for(int i = 0; i < nlocal; i++){
            x = get_index(local[i].x, matrixsize);
            y = get_index(local[i].y, matrixsize);
            local[i].ax = local[i].ay = 0;
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y-1].size(); k++){
                    apply_force(local[i], *matrix[x-1+j][y-1][k]);
                }
            }
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y].size(); k++){
                    apply_force(local[i], *matrix[x-1+j][y][k]);
                }
            }
            for(int j = 0; j < 3; j++){
                for(int k = 0; k < matrix[x-1+j][y+1].size(); k++){
                    apply_force(local[i], *matrix[x-1+j][y+1][k]);
                }
            }
        }
    
        for(int i = 0; i < matrixsize; i++){
            for(int j = 0; j < matrixsize; j++){
                matrix[i][j].clear();
            }
        }
        
        #endif

        //
        //  move particles
        //
        for( int i = 0; i < nlocal; i++ )
            move( local[i] );
    }
    simulation_time = read_timer( ) - simulation_time;
    
    if( rank == 0 )
        printf( "n = %d, n_procs = %d, simulation time = %g s\n", n, n_proc, simulation_time );
    
    //
    //  release resources
    //
    free( partition_offsets );
    free( partition_sizes );
    free( local );
    free( particles );
    if( fsave )
        fclose( fsave );
    
    MPI_Finalize( );
    
    return 0;
}
