/*********************************************************
 * Data file class
 * 
 * Changelog:
 *
 * 2009-08 Ben Dudson <bd512@york.ac.uk>
 *    * Added enable and wtime global properties
 *
 * 2009-07 Ben Dudson <bd512@york.ac.uk>
 *    * Adapted to handle PDB and/or NetCDF files
 *
 * 2009-04 Ben Dudson <bd512@york.ac.uk>
 *    * Initial version
 *
 **************************************************************************
 * Copyright 2010 B.D.Dudson, S.Farley, M.V.Umansky, X.Q.Xu
 *
 * Contact Ben Dudson, bd512@york.ac.uk
 * 
 * This file is part of BOUT++.
 *
 * BOUT++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BOUT++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with BOUT++.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *********************************************************/
#include "mpi.h" // For MPI_Wtime()

#include <datafile.hxx>
#include "formatfactory.hxx"

#include <globals.hxx>
#include <boutexception.hxx>



///////////////////////////////////////
// Global variables, shared between Datafile objects
bool Datafile::enabled = true;
BoutReal Datafile::wtime = 0.0;

Datafile::Datafile(DataFormat *format) : low_prec(false), file(NULL) {
  setFormat(format);
}

Datafile::~Datafile() {
  if(file != NULL)
    delete file;
}

void Datafile::setFormat(DataFormat *format) {
  if(file != NULL)
    delete file;
  
  file = format;
  
  if(low_prec)
    file->setLowPrecision();
}

void Datafile::setFormat(const string &format) {
  setFormat(FormatFactory::getInstance()->createDataFormat(format.c_str()));
}

void Datafile::setLowPrecision() {
  low_prec = true;
  file->setLowPrecision();
}

void Datafile::add(int &i, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);

  VarStr<int> d;

  d.ptr = &i;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  
  int_arr.push_back(d);
}

void Datafile::add(BoutReal &r, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);
  
  VarStr<BoutReal> d;

  d.ptr = &r;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  
  BoutReal_arr.push_back(d);
}

void Datafile::add(Field2D &f, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);
  
  VarStr<Field2D> d;

  d.ptr = &f;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  
  f2d_arr.push_back(d);
}

void Datafile::add(Field3D &f, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);
  
  VarStr<Field3D> d;

  d.ptr = &f;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  
  f3d_arr.push_back(d);
}

void Datafile::add(Vector2D &f, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);
  
  VarStr<Vector2D> d;

  d.ptr = &f;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  d.covar = f.covariant;
  
  v2d_arr.push_back(d);
}

void Datafile::add(Vector3D &f, const char *name, int grow) {
  if(varAdded(string(name)))
    throw BoutException("Variable '%s' already added to Datafile", name);
  
  VarStr<Vector3D> d;

  d.ptr = &f;
  d.name = string(name);
  d.grow = (grow > 0) ? true : false;
  d.covar = f.covariant;
  
  v3d_arr.push_back(d);
}

int Datafile::read(const char *format, ...) {
  va_list ap;  // List of arguments
  
  if(format == (const char*) NULL)
    return 1;

  char filename[512];
  va_start(ap, format);
    vsprintf(filename, format, ap);
  va_end(ap);

  // Record starting time
  BoutReal tstart = MPI_Wtime();
  
  // Open the file
  
  if(!file->openr(filename))
    return 1;

  if(!file->is_valid())
    return 1;  

  file->setRecord(-1); // Read the latest record

  // Read integers

  for(std::vector< VarStr<int> >::iterator it = int_arr.begin(); it != int_arr.end(); it++) {
    if(it->grow) {
      if(!file->read_rec(it->ptr, it->name.c_str())) {
	output.write("\tWARNING: Could not read integer %s. Setting to zero\n", it->name.c_str());
	*(it->ptr) = 0;
	continue;
      }
    }else {
      if(!file->read(it->ptr, it->name.c_str())) {
	output.write("\tWARNING: Could not read integer %s. Setting to zero\n", it->name.c_str());
	*(it->ptr) = 0;
	continue;
      }
    }
  }

  // Read BoutReals

  for(std::vector< VarStr<BoutReal> >::iterator it = BoutReal_arr.begin(); it != BoutReal_arr.end(); it++) {
    if(it->grow) {
      if(!file->read_rec(it->ptr, it->name)) {
	output.write("\tWARNING: Could not read BoutReal %s. Setting to zero\n", it->name.c_str());
	*(it->ptr) = 0;
	continue;
      }
    }else {
      if(!file->read(it->ptr, it->name)) {
	output.write("\tWARNING: Could not read BoutReal %s. Setting to zero\n", it->name.c_str());
	*(it->ptr) = 0;
	continue;
      }
    }
  }
  
  // Read 2D fields
  
  for(std::vector< VarStr<Field2D> >::iterator it = f2d_arr.begin(); it != f2d_arr.end(); it++) {
    read_f2d(it->name, it->ptr, it->grow);
  }

  // Read 3D fields
  
  for(std::vector< VarStr<Field3D> >::iterator it = f3d_arr.begin(); it != f3d_arr.end(); it++) {
    read_f3d(it->name, it->ptr, it->grow);
  }

  // 2D vectors
  
  for(std::vector< VarStr<Vector2D> >::iterator it = v2d_arr.begin(); it != v2d_arr.end(); it++) {
    if(it->covar) {
      // Reading covariant vector
      read_f2d(it->name+string("_x"), &(it->ptr->x), it->grow);
      read_f2d(it->name+string("_y"), &(it->ptr->y), it->grow);
      read_f2d(it->name+string("_z"), &(it->ptr->z), it->grow);
    }else {
      read_f2d(it->name+string("x"), &(it->ptr->x), it->grow);
      read_f2d(it->name+string("y"), &(it->ptr->y), it->grow);
      read_f2d(it->name+string("z"), &(it->ptr->z), it->grow);
    }

    it->ptr->covariant = it->covar;
  }

  // 3D vectors
  
  for(std::vector< VarStr<Vector3D> >::iterator it = v3d_arr.begin(); it != v3d_arr.end(); it++) {
    if(it->covar) {
      // Reading covariant vector
      read_f3d(it->name+string("_x"), &(it->ptr->x), it->grow);
      read_f3d(it->name+string("_y"), &(it->ptr->y), it->grow);
      read_f3d(it->name+string("_z"), &(it->ptr->z), it->grow);
    }else {
      read_f3d(it->name+string("x"), &(it->ptr->x), it->grow);
      read_f3d(it->name+string("y"), &(it->ptr->y), it->grow);
      read_f3d(it->name+string("z"), &(it->ptr->z), it->grow);
    }

    it->ptr->covariant = it->covar;
  }
  
  file->close();

  wtime += MPI_Wtime() - tstart;

  return 0;
}

int Datafile::write(const char *format, ...)
{
  va_list ap;  // List of arguments
  
  if(format == (const char*) NULL)
    return 1;

  char filename[512];
  va_start(ap, format);
    vsprintf(filename, format, ap);
  va_end(ap);
  
  if(write(string(filename), false))
    return 0;
  
  return 1;
}

int Datafile::append(const char *format, ...)
{
  va_list ap;  // List of arguments
  
  if(format == (const char*) NULL)
    return 1;
  
  char filename[512];
  va_start(ap, format);
    vsprintf(filename, format, ap);
  va_end(ap);
  
  if(write(string(filename), true))
    return 0;
  
  return 1;
}

bool Datafile::write(const string &filename, bool append)
{
  if(!enabled)
    return true; // Just pretend it worked

  // Record starting time
  BoutReal tstart = MPI_Wtime();

  if(!file->openw(filename, append))
    return false;

  if(!file->is_valid())
    return false;
  
  file->setRecord(-1); // Latest record

  // Write integers
  for(std::vector< VarStr<int> >::iterator it = int_arr.begin(); it != int_arr.end(); it++) {
    if(it->grow) {
      file->write_rec(it->ptr, it->name);
    }else {
      file->write(it->ptr, it->name);
    }
  }
  
  // Write BoutReals
  for(std::vector< VarStr<BoutReal> >::iterator it = BoutReal_arr.begin(); it != BoutReal_arr.end(); it++) {
    if(it->grow) {
      file->write_rec(it->ptr, it->name);
    }else {
      file->write(it->ptr, it->name);
    }
  }

  // Write 2D fields
  
  for(std::vector< VarStr<Field2D> >::iterator it = f2d_arr.begin(); it != f2d_arr.end(); it++) {
    write_f2d(it->name, it->ptr, it->grow);
  }

  // Write 3D fields
  
  for(std::vector< VarStr<Field3D> >::iterator it = f3d_arr.begin(); it != f3d_arr.end(); it++) {
    write_f3d(it->name, it->ptr, it->grow);
  }
  
  // 2D vectors
  
  for(std::vector< VarStr<Vector2D> >::iterator it = v2d_arr.begin(); it != v2d_arr.end(); it++) {
    if(it->covar) {
      // Writing covariant vector
      Vector2D v  = *(it->ptr);
      v.toCovariant();
      
      write_f2d(it->name+string("_x"), &(v.x), it->grow);
      write_f2d(it->name+string("_y"), &(v.y), it->grow);
      write_f2d(it->name+string("_z"), &(v.z), it->grow);
    }else {
      // Writing contravariant vector
      Vector2D v  = *(it->ptr);
      v.toContravariant();
      
      write_f2d(it->name+string("x"), &(v.x), it->grow);
      write_f2d(it->name+string("y"), &(v.y), it->grow);
      write_f2d(it->name+string("z"), &(v.z), it->grow);
    }
  }

  // 3D vectors
  
  for(std::vector< VarStr<Vector3D> >::iterator it = v3d_arr.begin(); it != v3d_arr.end(); it++) {
    if(it->covar) {
      // Writing covariant vector
      Vector3D v  = *(it->ptr);
      v.toCovariant();
      
      write_f3d(it->name+string("_x"), &(v.x), it->grow);
      write_f3d(it->name+string("_y"), &(v.y), it->grow);
      write_f3d(it->name+string("_z"), &(v.z), it->grow);
    }else {
      // Writing contravariant vector
      Vector3D v  = *(it->ptr);
      v.toContravariant();
      
      write_f3d(it->name+string("x"), &(v.x), it->grow);
      write_f3d(it->name+string("y"), &(v.y), it->grow);
      write_f3d(it->name+string("z"), &(v.z), it->grow);
    }
  }

  file->close();

  wtime += MPI_Wtime() - tstart;

  return true;
}

void Datafile::setFilename(const char *format, ...)
{
  va_list ap;  // List of arguments
  
  if(format == (const char*) NULL) {
    def_filename.clear();
    return;
  }
  
  char filename[512];
  va_start(ap, format);
    vsprintf(filename, format, ap);
  va_end(ap);
  
  setFilename(string(filename));
}

/////////////////////////////////////////////////////////////

bool Datafile::read_f2d(const string &name, Field2D *f, bool grow)
{
  f->allocate();
  
  if(grow) {
    if(!file->read_rec(*(f->getData()), name, mesh->ngx, mesh->ngy)) {
      output.write("\tWARNING: Could not read 2D field %s. Setting to zero\n", name.c_str());
      *f = 0.0;
      return false;
    }
  }else {
    if(!file->read(*(f->getData()), name, mesh->ngx, mesh->ngy)) {
      output.write("\tWARNING: Could not read 2D field %s. Setting to zero\n", name.c_str());
      *f = 0.0;
      return false;
    }
  }
  return true;
}

bool Datafile::read_f3d(const string &name, Field3D *f, bool grow)
{
  f->allocate();
  
  if(grow) {
    if(!file->read_rec(**(f->getData()), name, mesh->ngx, mesh->ngy, mesh->ngz)) {
      output.write("\tWARNING: Could not read 3D field %s. Setting to zero\n", name.c_str());
      *f = 0.0;
      return false;
    }
  }else {
    if(!file->read(**(f->getData()), name, mesh->ngx, mesh->ngy, mesh->ngz)) {
      output.write("\tWARNING: Could not read 3D field %s. Setting to zero\n", name.c_str());
      *f = 0.0;
      return false;
    }
  }
  return true;
}

bool Datafile::write_f2d(const string &name, Field2D *f, bool grow)
{
  if(!f->isAllocated())
    return false; // No data allocated
  
  if(grow) {
    return file->write_rec(*(f->getData()), name, mesh->ngx, mesh->ngy);
  }else {
    return file->write(*(f->getData()), name, mesh->ngx, mesh->ngy);
  }
}

bool Datafile::write_f3d(const string &name, Field3D *f, bool grow)
{
  if(!f->isAllocated()) {
    //output << "Datafile: unallocated: " << name << endl;
    return false; // No data allocated
  }

  if(grow) {
    return file->write_rec(**(f->getData()), name, mesh->ngx, mesh->ngy, mesh->ngz);
  }else {
    return file->write(**(f->getData()), name, mesh->ngx, mesh->ngy, mesh->ngz);
  }
}

bool Datafile::varAdded(const string &name) {
  for(std::vector< VarStr<int> >::iterator it = int_arr.begin(); it != int_arr.end(); it++) {
    if(name == it->name)
      return true;
  }

  for(std::vector< VarStr<BoutReal> >::iterator it = BoutReal_arr.begin(); it != BoutReal_arr.end(); it++) {
    if(name == it->name)
      return true;
  }

  for(std::vector< VarStr<Field2D> >::iterator it = f2d_arr.begin(); it != f2d_arr.end(); it++) {
    if(name == it->name)
      return true;
  }
  
  for(std::vector< VarStr<Field3D> >::iterator it = f3d_arr.begin(); it != f3d_arr.end(); it++) {
    if(name == it->name)
      return true;
  }
  
  for(std::vector< VarStr<Vector2D> >::iterator it = v2d_arr.begin(); it != v2d_arr.end(); it++) {
    if(name == it->name)
      return true;
  }

  for(std::vector< VarStr<Vector3D> >::iterator it = v3d_arr.begin(); it != v3d_arr.end(); it++) {
    if(name == it->name)
      return true;
  }
  return false;
}
