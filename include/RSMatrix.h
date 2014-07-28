#ifndef RSMATRIX_H
#define RSMATRIX_H

#include <memory>
#include <vector>

class RSMatrix
{
private:
  size_t K_;
  size_t M_; 
  size_t W_; 
  std::vector<size_t> partW_; 
  std::vector<std::shared_ptr<int> > partMatrices_;

public:
  RSMatrix(size_t K, size_t M, size_t W); 

  size_t K() const; 
  size_t M() const; 
  size_t W() const; 

  size_t getNumParts() const;
  size_t getPartW(size_t index) const; 
  int* getMatrix(size_t index); 
  const int* getMatrix(size_t index) const; 
};

inline size_t RSMatrix::K() const
{
  return K_;
}

inline size_t RSMatrix::M() const
{
  return M_;
}

inline size_t RSMatrix::W() const
{
  return W_;
}

inline size_t RSMatrix::getNumParts() const
{
  return partW_.size();
}

inline size_t RSMatrix::getPartW(size_t index) const
{
  return partW_[index];
}

inline int* RSMatrix::getMatrix(size_t index)
{
  return partMatrices_[index].get(); 
}

inline const int* RSMatrix::getMatrix(size_t index) const
{
  return partMatrices_[index].get(); 
}

#endif // RSMATRIX_H
