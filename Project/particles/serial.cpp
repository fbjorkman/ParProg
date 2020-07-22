#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "common.h"
#include <vector>
#include <array>

//#define OLD

using namespace std;

//
//  benchmarking program
//
int main( int argc, char **argv )
{    
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
    
    FILE *fsave = savename ? fopen( savename, "w" ) : NULL;
    particle_t *particles = (particle_t*) malloc( n * sizeof(particle_t) );
    set_size( n );
    double size = get_size();
    int matrixsize = get_gridsize();
    init_particles( n, particles );
    // a matrix of vectors containg particles, with a empty "frame" around the matrix for edge cases
	vector<vector<vector<particle_t*>>> matrix;
	matrix.resize(matrixsize+2);
    for(int i = 0; i < matrixsize+2; i++){
        matrix[i].resize(matrixsize+2);
    }

    //
    //  simulate a number of time steps
    //
    double simulation_time = read_timer( );
    for( int step = 0; step < s; step++ )
    {
    	//printf("%d\n", step);
        //
        //  compute forces
        //
        #ifdef OLD
        for( int i = 0; i < n; i++ )
        {
            particles[i].ax = particles[i].ay = 0;
            for (int j = 0; j < n; j++ )
            	apply_force( particles[i], particles[j] );
        }
    	#endif
    	#ifndef OLD
        int x, y;
    	for(int i = 0; i < n; i++){
    		x = get_index(particles[i].x, matrixsize);
    		y = get_index(particles[i].y, matrixsize);
    		matrix[x][y].push_back(&particles[i]);
    	}

 		for(int i = 0; i < n; i++){
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
	    for(int i = 1; i < matrixsize+1; i++){
	    	for(int j = 1; j < matrixsize+1; j++){
	    		matrix[i][j].clear();
	    	}
	    }
    	#endif

        //
        //  move particles
        //
        for( int i = 0; i < n; i++ ){ 
            move( particles[i] );
        }
        //
        //  save if necessary
        //
        if( fsave && (step%f) == 0 )
            save( fsave, n, particles );
    }

    simulation_time = read_timer( ) - simulation_time;
    
    printf( "n = %d, simulation time = %g seconds\n", n, simulation_time );
    
    free( particles );
    if( fsave )
        fclose( fsave );
    
    return 0;
}
