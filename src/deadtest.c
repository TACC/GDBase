/* This file is part of GDBase.
 *
 * GDBase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GDBase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GDBase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Anthony DiGirolamo, Dan Stanzione, Karl Lindekugel
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) 
{
  int rank, size,i;
  double buf[1024];
  MPI_Status stat;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
  for (i=0; i < 1024; i++)	
    buf[i] = sqrt((double)i);

  //Send in a ring, except for 0, who recvs
  if (rank) 
    {
      //Send
      printf("%d: S->%d with %d of %s\n", rank, (rank+1)%size, 1024, "DOUBLE");
      MPI_Send(buf, 1024, MPI_DOUBLE, (rank + 1) % size, 0, MPI_COMM_WORLD);
    }
  else
    {
      //Recv
      printf("%d: R<-%d with %d of %s\n", rank, size-1, 1024, "DOUBLE");
      MPI_Recv(buf, 1024, MPI_DOUBLE, size-1, 0, MPI_COMM_WORLD, &stat);
    }

  //Reverse
  if (rank) 
    {
      //Recv
      if (rank != 511) 
        {
          MPI_Recv(buf, 1024, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &stat);
          printf("%d: R<-%d with %d of %s\n", rank, rank-1, 1024, "DOUBLE");
        }
    }
  else
    {
      //Send
      printf("%d: S->%d with %d of %s\n", rank, (rank+1)%size, 1024, "DOUBLE");
      MPI_Send(buf, 1024, MPI_DOUBLE, rank+1 , 0, MPI_COMM_WORLD);
      
    }

  MPI_Barrier(MPI_COMM_WORLD);
  
  if (!rank) printf("Done\n");

  MPI_Finalize();
}

