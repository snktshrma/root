// @(#)root/matrix:$Name:  $:$Id: TMatrixT.cxx,v 1.18 2006/05/18 18:57:11 brun Exp $
// Authors: Fons Rademakers, Eddy Offermann   Nov 2003

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TMatrixT                                                             //
//                                                                      //
// Template class of a general matrix in the linear algebra package     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <typeinfo>

#include "TMatrixT.h"
#include "TMatrixTSym.h"
#include "TMatrixTLazy.h"
#include "TMatrixTCramerInv.h"
#include "TDecompLU.h"
#include "TMatrixDEigen.h"
#include "TClass.h"

#ifndef R__ALPHA
templateClassImp(TMatrixT)
#endif
//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(Int_t no_rows,Int_t no_cols)
{
   Allocate(no_rows,no_cols,0,0,1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb)
{
   Allocate(row_upb-row_lwb+1,col_upb-col_lwb+1,row_lwb,col_lwb,1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(Int_t no_rows,Int_t no_cols,const Element *elements,Option_t *option)
{
// option="F": array elements contains the matrix stored column-wise
//             like in Fortran, so a[i,j] = elements[i+no_rows*j],
// else        it is supposed that array elements are stored row-wise
//             a[i,j] = elements[i*no_cols+j]
//
// array elements are copied

   Allocate(no_rows,no_cols);
   SetMatrixArray(elements,option);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb,
                            const Element *elements,Option_t *option)
{
// array elements are copied

   Allocate(row_upb-row_lwb+1,col_upb-col_lwb+1,row_lwb,col_lwb);
   SetMatrixArray(elements,option);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixT<Element> &another) : TMatrixTBase<Element>(another)
{
   R__ASSERT(another.IsValid());
   Allocate(another.GetNrows(),another.GetNcols(),another.GetRowLwb(),another.GetColLwb());
   *this = another;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixTSym<Element> &another)
{
   R__ASSERT(another.IsValid());
   Allocate(another.GetNrows(),another.GetNcols(),another.GetRowLwb(),another.GetColLwb());
   *this = another;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixTSparse<Element> &another)
{
   R__ASSERT(another.IsValid());
   Allocate(another.GetNrows(),another.GetNcols(),another.GetRowLwb(),another.GetColLwb());
   *this = another;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(EMatrixCreatorsOp1 op,const TMatrixT<Element> &prototype)
{
// Create a matrix applying a specific operation to the prototype.
// Example: TMatrixT<Element> a(10,12); ...; TMatrixT<Element> b(TMatrixT::kTransposed, a);
// Supported operations are: kZero, kUnit, kTransposed, kInverted and kAtA.

   R__ASSERT(this != &prototype);
   this->Invalidate();

   R__ASSERT(prototype.IsValid());

   switch(op) {
      case kZero:
         Allocate(prototype.GetNrows(),prototype.GetNcols(),
                  prototype.GetRowLwb(),prototype.GetColLwb(),1);
         break;

      case kUnit:
         Allocate(prototype.GetNrows(),prototype.GetNcols(),
                  prototype.GetRowLwb(),prototype.GetColLwb(),1);
         this->UnitMatrix();
         break;

      case kTransposed:
         Allocate(prototype.GetNcols(), prototype.GetNrows(),
                  prototype.GetColLwb(),prototype.GetRowLwb());
         Transpose(prototype);
         break;

      case kInverted:
      {
         Allocate(prototype.GetNrows(),prototype.GetNcols(),
                  prototype.GetRowLwb(),prototype.GetColLwb(),1);
         *this = prototype;
         // Since the user can not control the tolerance of this newly created matrix
         // we put it to the smallest possible number
         const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
         this->Invert();
         this->SetTol(oldTol);
         break;
      }

      case kAtA:
         Allocate(prototype.GetNcols(),prototype.GetNcols(),prototype.GetColLwb(),prototype.GetColLwb(),1);
         TMult(prototype,prototype);
         break;

      default:
         Error("TMatrixT(EMatrixCreatorOp1)", "operation %d not yet implemented", op);
   }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixT<Element> &a,EMatrixCreatorsOp2 op,const TMatrixT<Element> &b)
{
// Create a matrix applying a specific operation to two prototypes.
// Example: TMatrixT<Element> a(10,12), b(12,5); ...; TMatrixT<Element> c(a, TMatrixT::kMult, b);
// Supported operations are: kMult (a*b), kTransposeMult (a'*b), kInvMult (a^(-1)*b)

   this->Invalidate();

   R__ASSERT(a.IsValid());
   R__ASSERT(b.IsValid());

   switch(op) {
      case kMult:
         Allocate(a.GetNrows(),b.GetNcols(),a.GetRowLwb(),b.GetColLwb(),1);
         Mult(a,b);
         break;

      case kTransposeMult:
         Allocate(a.GetNcols(),b.GetNcols(),a.GetColLwb(),b.GetColLwb(),1);
         TMult(a,b);
         break;

      case kMultTranspose:
         Allocate(a.GetNrows(),b.GetNrows(),a.GetRowLwb(),b.GetRowLwb(),1);
         MultT(a,b);
         break;

      case kInvMult:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         *this = a;
         const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
         this->Invert();
         this->SetTol(oldTol);
         *this *= b;
         break;
      }

      case kPlus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Plus(a,b);
         break;
      }

      case kMinus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Minus(a,b);
         break;
      }

      default:
         Error("TMatrixT(EMatrixCreatorOp2)", "operation %d not yet implemented", op);
   }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixT<Element> &a,EMatrixCreatorsOp2 op,const TMatrixTSym<Element> &b)
{
   this->Invalidate();

   R__ASSERT(a.IsValid());
   R__ASSERT(b.IsValid());

   switch(op) {
      case kMult:
         Allocate(a.GetNrows(),b.GetNcols(),a.GetRowLwb(),b.GetColLwb(),1);
         Mult(a,b);
         break;

      case kTransposeMult:
         Allocate(a.GetNcols(),b.GetNcols(),a.GetColLwb(),b.GetColLwb(),1);
         TMult(a,b);
         break;

      case kMultTranspose:
         Allocate(a.GetNrows(),b.GetNrows(),a.GetRowLwb(),b.GetRowLwb(),1);
         MultT(a,b);
         break;

      case kInvMult:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         *this = a;
         const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
         this->Invert();
         this->SetTol(oldTol);
         *this *= b;
         break;
      }

      case kPlus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Plus(a,b);
         break;
      }

      case kMinus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Minus(a,b);
         break;
      }

      default:
         Error("TMatrixT(EMatrixCreatorOp2)", "operation %d not yet implemented", op);
   }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixTSym<Element> &a,EMatrixCreatorsOp2 op,const TMatrixT<Element> &b)
{
   this->Invalidate();

   R__ASSERT(a.IsValid());
   R__ASSERT(b.IsValid());

   switch(op) {
      case kMult:
         Allocate(a.GetNrows(),b.GetNcols(),a.GetRowLwb(),b.GetColLwb(),1);
         Mult(a,b);
         break;

      case kTransposeMult:
         Allocate(a.GetNcols(),b.GetNcols(),a.GetColLwb(),b.GetColLwb(),1);
         TMult(a,b);
         break;

      case kMultTranspose:
         Allocate(a.GetNrows(),b.GetNrows(),a.GetRowLwb(),b.GetRowLwb(),1);
         MultT(a,b);
         break;

      case kInvMult:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         *this = a;
         const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
         this->Invert();
         this->SetTol(oldTol);
         *this *= b;
         break;
      }

      case kPlus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Plus(a,b);
         break;
      }

      case kMinus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Minus(a,b);
         break;
      }

      default:
         Error("TMatrixT(EMatrixCreatorOp2)", "operation %d not yet implemented", op);
   }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixTSym<Element> &a,EMatrixCreatorsOp2 op,const TMatrixTSym<Element> &b)
{
   this->Invalidate();

   R__ASSERT(a.IsValid());
   R__ASSERT(b.IsValid());

   switch(op) {
      case kMult:
         Allocate(a.GetNrows(),b.GetNcols(),a.GetRowLwb(),b.GetColLwb(),1);
         Mult(a,b);
         break;

      case kTransposeMult:
         Allocate(a.GetNcols(),b.GetNcols(),a.GetColLwb(),b.GetColLwb(),1);
         TMult(a,b);
         break;

      case kMultTranspose:
         Allocate(a.GetNrows(),b.GetNrows(),a.GetRowLwb(),b.GetRowLwb(),1);
         MultT(a,b);
         break;

      case kInvMult:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         *this = a;
         const Element oldTol = this->SetTol(std::numeric_limits<Element>::min());
         this->Invert();
         this->SetTol(oldTol);
         *this *= b;
         break;
      }

      case kPlus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Plus(*dynamic_cast<const TMatrixT<Element> *>(&a),b);
         break;
      }

      case kMinus:
      {
         Allocate(a.GetNrows(),a.GetNcols(),a.GetRowLwb(),a.GetColLwb(),1);
         Minus(*dynamic_cast<const TMatrixT<Element> *>(&a),b);
         break;
      }

      default:
         Error("TMatrixT(EMatrixCreatorOp2)", "operation %d not yet implemented", op);
   }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element>::TMatrixT(const TMatrixTLazy<Element> &lazy_constructor)
{
   Allocate(lazy_constructor.GetRowUpb()-lazy_constructor.GetRowLwb()+1,
            lazy_constructor.GetColUpb()-lazy_constructor.GetColLwb()+1,
            lazy_constructor.GetRowLwb(),lazy_constructor.GetColLwb(),1);
   lazy_constructor.FillIn(*this);
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Delete_m(Int_t size,Element *&m)
{
// delete data pointer m, if it was assigned on the heap

   if (m) {
      if (size > this->kSizeMax)
         delete [] m;
      m = 0;
   }
}

//______________________________________________________________________________
template<class Element>
Element* TMatrixT<Element>::New_m(Int_t size)
{
// return data pointer . if requested size <= kSizeMax, assign pointer
// to the stack space

   if (size == 0) return 0;
   else {
      if ( size <= this->kSizeMax )
         return fDataStack;
      else {
         Element *heap = new Element[size];
         return heap;
      }
   }
}

//______________________________________________________________________________
template<class Element>
Int_t TMatrixT<Element>::Memcpy_m(Element *newp,const Element *oldp,Int_t copySize,
                                  Int_t newSize,Int_t oldSize)
{
  // copy copySize doubles from *oldp to *newp . However take care of the
  // situation where both pointers are assigned to the same stack space

   if (copySize == 0 || oldp == newp)
      return 0;
   else {
      if ( newSize <= this->kSizeMax && oldSize <= this->kSizeMax ) {
         // both pointers are inside fDataStack, be careful with copy direction !
         if (newp > oldp) {
            for (Int_t i = copySize-1; i >= 0; i--)
               newp[i] = oldp[i];
         } else {
            for (Int_t i = 0; i < copySize; i++)
               newp[i] = oldp[i];
         }
      }
      else
         memcpy(newp,oldp,copySize*sizeof(Element));
   }
   return 0;
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Allocate(Int_t no_rows,Int_t no_cols,Int_t row_lwb,Int_t col_lwb,
                                 Int_t init,Int_t /*nr_nonzeros*/)
{
  // Allocate new matrix. Arguments are number of rows, columns, row
  // lowerbound (0 default) and column lowerbound (0 default).

   if (no_rows < 0 || no_cols < 0)
   {
      Error("Allocate","no_rows=%d no_cols=%d",no_rows,no_cols);
      this->Invalidate();
      return;
   }

   this->MakeValid();
   this->fNrows   = no_rows;
   this->fNcols   = no_cols;
   this->fRowLwb  = row_lwb;
   this->fColLwb  = col_lwb;
   this->fNelems  = this->fNrows*this->fNcols;
   this->fIsOwner = kTRUE;
   this->fTol     = std::numeric_limits<Element>::epsilon();

   if (this->fNelems > 0) {
      fElements = New_m(this->fNelems);
      if (init)
         memset(fElements,0,this->fNelems*sizeof(Element));
   } else
     fElements = 0;
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Plus(const TMatrixT<Element> &a,const TMatrixT<Element> &b)
{
  // General matrix summation. Create a matrix C such that C = A + B.

   if (gMatrixCheck) {
      if (!AreCompatible(a,b)) {
         Error("Plus","matrices not compatible");
         return;
      }

      if (this == &a) {
         Error("Plus","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("Plus","this == &b");
         this->Invalidate();
         return;
      }
   }

   const Element *       ap      = a.GetMatrixArray();
   const Element *       bp      = b.GetMatrixArray();
         Element *       cp      = this->GetMatrixArray();
   const Element * const cp_last = cp+this->fNelems;

   while (cp < cp_last) {
       *cp = *ap++ + *bp++;
       cp++;
   }
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Plus(const TMatrixT<Element> &a,const TMatrixTSym<Element> &b)
{
  // General matrix summation. Create a matrix C such that C = A + B.

   if (gMatrixCheck) {
      if (!AreCompatible(a,b)) {
         Error("Plus","matrices not compatible");
         return;
      }

      if (this == &a) {
         Error("Plus","this == &a");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&b)) {
          Error("Plus","this == &b");
          this->Invalidate();
          return;
      }
   }

   const Element *       ap      = a.GetMatrixArray();
   const Element *       bp      = b.GetMatrixArray();
         Element *       cp      = this->GetMatrixArray();
   const Element * const cp_last = cp+this->fNelems;

   while (cp < cp_last) {
       *cp = *ap++ + *bp++;
       cp++;
   }
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Minus(const TMatrixT<Element> &a,const TMatrixT<Element> &b)
{
  // General matrix summation. Create a matrix C such that C = A + B.

   if (gMatrixCheck) {
      if (!AreCompatible(a,b)) {
         Error("Minus","matrices not compatible");
         return;
      }

      if (this == &a) {
         Error("Minus","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("Minus","this == &b");
         this->Invalidate();
         return;
      }
   }

   const Element *       ap      = a.GetMatrixArray();
   const Element *       bp      = b.GetMatrixArray();
         Element *       cp      = this->GetMatrixArray();
   const Element * const cp_last = cp+this->fNelems;

   while (cp < cp_last) {
      *cp = *ap++ - *bp++;
      cp++;
   }
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Minus(const TMatrixT<Element> &a,const TMatrixTSym<Element> &b)
{
  // General matrix summation. Create a matrix C such that C = A + B.

   if (gMatrixCheck) {
      if (!AreCompatible(a,b)) {
         Error("Minus","matrices not compatible");
         return;
      }

      if (this == &a) {
         Error("Minus","this == &a");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&b)) {
         Error("Minus","this == &b");
         this->Invalidate();
         return;
      }
   }

   const Element *       ap      = a.GetMatrixArray();
   const Element *       bp      = b.GetMatrixArray();
         Element *       cp      = this->GetMatrixArray();
   const Element * const cp_last = cp+this->fNelems;

   while (cp < cp_last) {
       *cp = *ap++ - *bp++;
       cp++;
   }
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Mult(const TMatrixT<Element> &a,const TMatrixT<Element> &b)
{
  // General matrix multiplication. Create a matrix C such that C = A * B.

   if (gMatrixCheck) {
      if (a.GetNcols() != b.GetNrows() || a.GetColLwb() != b.GetRowLwb()) {
         Error("Mult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == &a) {
         Error("Mult","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("Mult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dgemm (CblasRowMajor,CblasNoTrans,CblasNoTrans,fNrows,fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_sgemm (CblasRowMajor,CblasNoTrans,CblasNoTrans,fNrows,fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else
      Error("Mult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultB(ap,na,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Mult(const TMatrixTSym<Element> &a,const TMatrixT<Element> &b)
{
  // Matrix multiplication, with A symmetric and B general.
  // Create a matrix C such that C = A * B.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNcols() != b.GetNrows() || a.GetColLwb() != b.GetRowLwb()) {
         Error("Mult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&a)) {
         Error("Mult","this == &a");
         this->Invalidate();
        return;
      }

      if (this == &b) {
         Error("Mult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dsymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                   ap,a.GetNcols(),bp,b.GetNcols(),0.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_ssymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                   ap,a.GetNcols(),bp,b.GetNcols(),0.0,cp,fNcols);
   else
      Error("Mult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultB(ap,na,ncolsa,bp,nb,ncolsb,cp);

#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Mult(const TMatrixT<Element> &a,const TMatrixTSym<Element> &b)
{
  // Matrix multiplication, with A general and B symmetric.
  // Create a matrix C such that C = A * B.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNcols() != b.GetNrows() || a.GetColLwb() != b.GetRowLwb()) {
         Error("Mult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == &a) {
         Error("Mult","this == &a");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&b)) {
         Error("Mult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dsymm (CblasRowMajor,CblasRight,CblasUpper,fNrows,fNcols,1.0,
                   bp,b.GetNcols(),ap,a.GetNcols(),0.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_ssymm (CblasRowMajor,CblasRight,CblasUpper,fNrows,fNcols,1.0,
                   bp,b.GetNcols(),ap,a.GetNcols(),0.0,cp,fNcols);
   else
      Error("Mult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultB(ap,na,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Mult(const TMatrixTSym<Element> &a,const TMatrixTSym<Element> &b)
{
  // Matrix multiplication, with A symmetric and B symmetric.
  // (Actually copied for the moment routine for B general)
  // Create a matrix C such that C = A * B.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNcols() != b.GetNrows() || a.GetColLwb() != b.GetRowLwb()) {
         Error("Mult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&a)) {
         Error("Mult","this == &a");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&b)) {
         Error("Mult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dsymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                   ap,a.GetNcols(),bp,b.GetNcols(),0.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_ssymm (CblasRowMajor,CblasLeft,CblasUpper,fNrows,fNcols,1.0,
                   ap,a.GetNcols(),bp,b.GetNcols(),0.0,cp,fNcols);
   else
      Error("Mult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultB(ap,na,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::TMult(const TMatrixT<Element> &a,const TMatrixT<Element> &b)
{
  // Create a matrix C such that C = A' * B. In other words,
  // c[i,j] = SUM{ a[k,i] * b[k,j] }.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNrows() != b.GetNrows() || a.GetRowLwb() != b.GetRowLwb()) {
         Error("TMult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == &a) {
         Error("TMult","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("TMult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dgemm (CblasRowMajor,CblasTrans,CblasNoTrans,this->fNrows,this->fNcols,a.GetNrows(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,this->fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_sgemm (CblasRowMajor,CblasTrans,CblasNoTrans,fNrows,fNcols,a.GetNrows(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else
      Error("TMult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AtMultB(ap,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::TMult(const TMatrixT<Element> &a,const TMatrixTSym<Element> &b)
{
  // Create a matrix C such that C = A' * B. In other words,
  // c[i,j] = SUM{ a[k,i] * b[k,j] }.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNrows() != b.GetNrows() || a.GetRowLwb() != b.GetRowLwb()) {
         Error("TMult","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == &a) {
         Error("TMult","this == &a");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&b)) {
         Error("TMult","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dgemm (CblasRowMajor,CblasTrans,CblasNoTrans,fNrows,fNcols,a.GetNrows(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_sgemm (CblasRowMajor,CblasTrans,CblasNoTrans,fNrows,fNcols,a.GetNrows(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else
      Error("TMult","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AtMultB(ap,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::MultT(const TMatrixT<Element> &a,const TMatrixT<Element> &b)
{
  // General matrix multiplication. Create a matrix C such that C = A * B^T.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());

      if (a.GetNcols() != b.GetNcols() || a.GetColLwb() != b.GetColLwb()) {
         Error("MultT","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == &a) {
         Error("MultT","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("MultT","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dgemm (CblasRowMajor,CblasNoTrans,CblasTrans,fNrows,fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_sgemm (CblasRowMajor,CblasNoTrans,CblasTrans,fNrows,fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else
      Error("MultT","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultBt(ap,na,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::MultT(const TMatrixTSym<Element> &a,const TMatrixT<Element> &b)
{
  // Matrix multiplication, with A symmetric and B general.
  // Create a matrix C such that C = A * B^T.

   if (gMatrixCheck) {
      R__ASSERT(a.IsValid());
      R__ASSERT(b.IsValid());
      if (a.GetNcols() != b.GetNcols() || a.GetColLwb() != b.GetColLwb()) {
         Error("MultT","A rows and B columns incompatible");
         this->Invalidate();
         return;
      }

      if (this == dynamic_cast<const TMatrixT<Element> *>(&a)) {
         Error("MultT","this == &a");
         this->Invalidate();
         return;
      }

      if (this == &b) {
         Error("MultT","this == &b");
         this->Invalidate();
         return;
      }
   }

#ifdef CBLAS
   const Element *ap = a.GetMatrixArray();
   const Element *bp = b.GetMatrixArray();
         Element *cp = this->GetMatrixArray();
   if (typeid(Element) == typeid(Double_t))
      cblas_dgemm (CblasRowMajor,CblasNoTrans,CblasTrans,this->fNrows,this->fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,this->fNcols);
   else if (typeid(Element) != typeid(Float_t))
      cblas_sgemm (CblasRowMajor,CblasNoTrans,CblasTrans,fNrows,fNcols,a.GetNcols(),
                   1.0,ap,a.GetNcols(),bp,b.GetNcols(),1.0,cp,fNcols);
   else
      Error("MultT","type %s not implemented in BLAS library",typeid(Element));
#else
   const Int_t na     = a.GetNoElements();
   const Int_t nb     = b.GetNoElements();
   const Int_t ncolsa = a.GetNcols();
   const Int_t ncolsb = b.GetNcols();
   const Element * const ap = a.GetMatrixArray();
   const Element * const bp = b.GetMatrixArray();
         Element *       cp = this->GetMatrixArray();

   AMultBt(ap,na,ncolsa,bp,nb,ncolsb,cp);
#endif
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::Use(Int_t row_lwb,Int_t row_upb,
                                          Int_t col_lwb,Int_t col_upb,Element *data)
{
   if (gMatrixCheck) {
      if (row_upb < row_lwb)
      {
         Error("Use","row_upb=%d < row_lwb=%d",row_upb,row_lwb);
         this->Invalidate();
         return *this;
      }
      if (col_upb < col_lwb)
      {
         Error("Use","col_upb=%d < col_lwb=%d",col_upb,col_lwb);
         this->Invalidate();
         return *this;
      }
   }

   Clear();
   this->fNrows    = row_upb-row_lwb+1;
   this->fNcols    = col_upb-col_lwb+1;
   this->fRowLwb   = row_lwb;
   this->fColLwb   = col_lwb;
   this->fNelems   = this->fNrows*this->fNcols;
         fElements = data;
   this->fIsOwner  = kFALSE;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixTBase<Element> &TMatrixT<Element>::GetSub(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb,
                                                 TMatrixTBase<Element> &target,Option_t *option) const
{
  // Get submatrix [row_lwb..row_upb][col_lwb..col_upb]; The indexing range of the
  // returned matrix depends on the argument option:
  //
  // option == "S" : return [0..row_upb-row_lwb+1][0..col_upb-col_lwb+1] (default)
  // else          : return [row_lwb..row_upb][col_lwb..col_upb]

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
         Error("GetSub","row_lwb out of bounds");
         target.Invalidate();
         return target;
      }
      if (col_lwb < this->fColLwb || col_lwb > this->fColLwb+this->fNcols-1) {
         Error("GetSub","col_lwb out of bounds");
         target.Invalidate();
         return target;
      }
      if (row_upb < this->fRowLwb || row_upb > this->fRowLwb+this->fNrows-1) {
         Error("GetSub","row_upb out of bounds");
         target.Invalidate();
         return target;
      }
      if (col_upb < this->fColLwb || col_upb > this->fColLwb+this->fNcols-1) {
         Error("GetSub","col_upb out of bounds");
         target.Invalidate();
         return target;
      }
      if (row_upb < row_lwb || col_upb < col_lwb) {
         Error("GetSub","row_upb < row_lwb || col_upb < col_lwb");
         target.Invalidate();
         return target;
      }
   }

   TString opt(option);
   opt.ToUpper();
   const Int_t shift = (opt.Contains("S")) ? 1 : 0;

   const Int_t row_lwb_sub = (shift) ? 0               : row_lwb;
   const Int_t row_upb_sub = (shift) ? row_upb-row_lwb : row_upb;
   const Int_t col_lwb_sub = (shift) ? 0               : col_lwb;
   const Int_t col_upb_sub = (shift) ? col_upb-col_lwb : col_upb;

   target.ResizeTo(row_lwb_sub,row_upb_sub,col_lwb_sub,col_upb_sub);
   const Int_t nrows_sub = row_upb_sub-row_lwb_sub+1;
   const Int_t ncols_sub = col_upb_sub-col_lwb_sub+1;

   if (target.GetRowIndexArray() && target.GetColIndexArray()) {
      for (Int_t irow = 0; irow < nrows_sub; irow++) {
         for (Int_t icol = 0; icol < ncols_sub; icol++) {
            target(irow+row_lwb_sub,icol+col_lwb_sub) = (*this)(row_lwb+irow,col_lwb+icol);
         }
      }
   } else {
      const Element *ap = this->GetMatrixArray()+(row_lwb-this->fRowLwb)*this->fNcols+(col_lwb-this->fColLwb);
            Element *bp = target.GetMatrixArray();

      for (Int_t irow = 0; irow < nrows_sub; irow++) {
         const Element *ap_sub = ap;
         for (Int_t icol = 0; icol < ncols_sub; icol++) {
            *bp++ = *ap_sub++;
         }
         ap += this->fNcols;
      }
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixTBase<Element> &TMatrixT<Element>::SetSub(Int_t row_lwb,Int_t col_lwb,const TMatrixTBase<Element> &source)
{
  // Insert matrix source starting at [row_lwb][col_lwb], thereby overwriting the part
  // [row_lwb..row_lwb+nrows_source][col_lwb..col_lwb+ncols_source];

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(source.IsValid());

      if (row_lwb < this->fRowLwb || row_lwb > this->fRowLwb+this->fNrows-1) {
         Error("SetSub","row_lwb outof bounds");
         this->Invalidate();
         return *this;
      }
      if (col_lwb < this->fColLwb || col_lwb > this->fColLwb+this->fNcols-1) {
         Error("SetSub","col_lwb outof bounds");
         this->Invalidate();
         return *this;
      }
      if (row_lwb+source.GetNrows() > this->fRowLwb+this->fNrows ||
            col_lwb+source.GetNcols() > this->fColLwb+this->fNcols) {
         Error("SetSub","source matrix too large");
         this->Invalidate();
         return *this;
      }
   }

   const Int_t nRows_source = source.GetNrows();
   const Int_t nCols_source = source.GetNcols();

   if (source.GetRowIndexArray() && source.GetColIndexArray()) {
      const Int_t rowlwb_s = source.GetRowLwb();
      const Int_t collwb_s = source.GetColLwb();
      for (Int_t irow = 0; irow < nRows_source; irow++) {
         for (Int_t icol = 0; icol < nCols_source; icol++) {
            (*this)(row_lwb+irow,col_lwb+icol) = source(rowlwb_s+irow,collwb_s+icol);
         }
      }
   } else {
      const Element *bp = source.GetMatrixArray();
            Element *ap = this->GetMatrixArray()+(row_lwb-this->fRowLwb)*this->fNcols+(col_lwb-this->fColLwb);

      for (Int_t irow = 0; irow < nRows_source; irow++) {
         Element *ap_sub = ap;
         for (Int_t icol = 0; icol < nCols_source; icol++) {
            *ap_sub++ = *bp++;
         }
         ap += this->fNcols;
      }
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixTBase<Element> &TMatrixT<Element>::ResizeTo(Int_t nrows,Int_t ncols,Int_t /*nr_nonzeros*/)
{
  // Set size of the matrix to nrows x ncols
  // New dynamic elements are created, the overlapping part of the old ones are
  // copied to the new structures, then the old elements are deleted.

   R__ASSERT(this->IsValid());
   if (!this->fIsOwner) {
      Error("ResizeTo(Int_t,Int_t)","Not owner of data array,cannot resize");
      this->Invalidate();
      return *this;
   }

   if (this->fNelems > 0) {
      if (this->fNrows == nrows && this->fNcols == ncols)
         return *this;
      else if (nrows == 0 || ncols == 0) {
         this->fNrows = nrows; this->fNcols = ncols;
         Clear();
         return *this;
      }

      Element    *elements_old = GetMatrixArray();
      const Int_t nelems_old   = this->fNelems;
      const Int_t nrows_old    = this->fNrows;
      const Int_t ncols_old    = this->fNcols;

      Allocate(nrows,ncols);
      R__ASSERT(this->IsValid());

      Element *elements_new = GetMatrixArray();
      // new memory should be initialized but be careful ot to wipe out the stack
      // storage. Initialize all when old or new storage was on the heap
      if (this->fNelems > this->kSizeMax || nelems_old > this->kSizeMax)
         memset(elements_new,0,this->fNelems*sizeof(Element));
      else if (this->fNelems > nelems_old)
         memset(elements_new+nelems_old,0,(this->fNelems-nelems_old)*sizeof(Element));

      // Copy overlap
      const Int_t ncols_copy = TMath::Min(this->fNcols,ncols_old);
      const Int_t nrows_copy = TMath::Min(this->fNrows,nrows_old);

      const Int_t nelems_new = this->fNelems;
      if (ncols_old < this->fNcols) {
         for (Int_t i = nrows_copy-1; i >= 0; i--)
            Memcpy_m(elements_new+i*this->fNcols,elements_old+i*ncols_old,ncols_copy,
                     nelems_new,nelems_old);
      } else {
         for (Int_t i = 0; i < nrows_copy; i++)
            Memcpy_m(elements_new+i*this->fNcols,elements_old+i*ncols_old,ncols_copy,
                     nelems_new,nelems_old);
      }

      Delete_m(nelems_old,elements_old);
   } else {
      Allocate(nrows,ncols,0,0,1);
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixTBase<Element> &TMatrixT<Element>::ResizeTo(Int_t row_lwb,Int_t row_upb,Int_t col_lwb,Int_t col_upb,
                                                   Int_t /*nr_nonzeros*/)
{
  // Set size of the matrix to [row_lwb:row_upb] x [col_lwb:col_upb]
  // New dynamic elemenst are created, the overlapping part of the old ones are
  // copied to the new structures, then the old elements are deleted.

   R__ASSERT(this->IsValid());
   if (!this->fIsOwner) {
      Error("ResizeTo(Int_t,Int_t,Int_t,Int_t)","Not owner of data array,cannot resize");
      this->Invalidate();
      return *this;
   }

   const Int_t new_nrows = row_upb-row_lwb+1;
   const Int_t new_ncols = col_upb-col_lwb+1;

   if (this->fNelems > 0) {

      if (this->fNrows  == new_nrows  && this->fNcols  == new_ncols &&
           this->fRowLwb == row_lwb    && this->fColLwb == col_lwb)
          return *this;
      else if (new_nrows == 0 || new_ncols == 0) {
         this->fNrows = new_nrows; this->fNcols = new_ncols;
         this->fRowLwb = row_lwb; this->fColLwb = col_lwb;
         Clear();
         return *this;
      }

      Element    *elements_old = GetMatrixArray();
      const Int_t nelems_old   = this->fNelems;
      const Int_t nrows_old    = this->fNrows;
      const Int_t ncols_old    = this->fNcols;
      const Int_t rowLwb_old   = this->fRowLwb;
      const Int_t colLwb_old   = this->fColLwb;

      Allocate(new_nrows,new_ncols,row_lwb,col_lwb);
      R__ASSERT(this->IsValid());

      Element *elements_new = GetMatrixArray();
      // new memory should be initialized but be careful ot to wipe out the stack
      // storage. Initialize all when old or new storag ewas on the heap
      if (this->fNelems > this->kSizeMax || nelems_old > this->kSizeMax)
         memset(elements_new,0,this->fNelems*sizeof(Element));
      else if (this->fNelems > nelems_old)
         memset(elements_new+nelems_old,0,(this->fNelems-nelems_old)*sizeof(Element));

      // Copy overlap
      const Int_t rowLwb_copy = TMath::Max(this->fRowLwb,rowLwb_old);
      const Int_t colLwb_copy = TMath::Max(this->fColLwb,colLwb_old);
      const Int_t rowUpb_copy = TMath::Min(this->fRowLwb+this->fNrows-1,rowLwb_old+nrows_old-1);
      const Int_t colUpb_copy = TMath::Min(this->fColLwb+this->fNcols-1,colLwb_old+ncols_old-1);

      const Int_t nrows_copy = rowUpb_copy-rowLwb_copy+1;
      const Int_t ncols_copy = colUpb_copy-colLwb_copy+1;

      if (nrows_copy > 0 && ncols_copy > 0) {
         const Int_t colOldOff = colLwb_copy-colLwb_old;
         const Int_t colNewOff = colLwb_copy-this->fColLwb;
         if (ncols_old < this->fNcols) {
            for (Int_t i = nrows_copy-1; i >= 0; i--) {
               const Int_t iRowOld = rowLwb_copy+i-rowLwb_old;
               const Int_t iRowNew = rowLwb_copy+i-this->fRowLwb;
               Memcpy_m(elements_new+iRowNew*this->fNcols+colNewOff,
                        elements_old+iRowOld*ncols_old+colOldOff,ncols_copy,this->fNelems,nelems_old);
            }
         } else {
            for (Int_t i = 0; i < nrows_copy; i++) {
               const Int_t iRowOld = rowLwb_copy+i-rowLwb_old;
               const Int_t iRowNew = rowLwb_copy+i-this->fRowLwb;
               Memcpy_m(elements_new+iRowNew*this->fNcols+colNewOff,
                        elements_old+iRowOld*ncols_old+colOldOff,ncols_copy,this->fNelems,nelems_old);
            }
         }
      }

      Delete_m(nelems_old,elements_old);
   } else {
      Allocate(new_nrows,new_ncols,row_lwb,col_lwb,1);
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
Double_t TMatrixT<Element>::Determinant() const
{
   const TMatrixT<Element> &tmp = *this;
   TDecompLU lu(tmp,this->fTol);
   Double_t d1,d2;
   lu.Det(d1,d2);
   return d1*TMath::Power(2.0,d2);
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Determinant(Double_t &d1,Double_t &d2) const
{
   const TMatrixT<Element> &tmp = *this;
   TDecompLU lu(tmp,Double_t(this->fTol));
   lu.Det(d1,d2);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::Invert(Double_t *det)
{
  // Invert the matrix and calculate its determinant

   R__ASSERT(this->IsValid());
   if (typeid(Element) == typeid(Double_t))
      TDecompLU::InvertLU(*dynamic_cast<TMatrixD *>(this),Double_t(this->fTol),det);
   else {
      TMatrixD tmp(*this);
      TDecompLU::InvertLU(tmp,Double_t(this->fTol),det);
      const Double_t *p1 = tmp.GetMatrixArray();
            Element  *p2 = this->GetMatrixArray();
      for (Int_t i = 0; i < this->GetNoElements(); i++)
         p2[i] = p1[i];
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::InvertFast(Double_t *det)
{
  // Invert the matrix and calculate its determinant

   R__ASSERT(this->IsValid());

   const Char_t nRows = Char_t(this->GetNrows());
   switch (nRows) {
      case 1:
      {
         if (this->GetNrows() != this->GetNcols() || this->GetRowLwb() != this->GetColLwb()) {
             Error("Invert()","matrix should be square");
             this->Invalidate();
         } else {
            Element *pM = this->GetMatrixArray();
            if (*pM == 0.) {
               Error("InvertFast","matrix is singular");
               this->Invalidate();
               *det = 0;
            }
            else {
               *det = *pM;
               *pM = 1.0/(*pM);
            }
         }
         return *this;
      }
      case 2:
      {
         TMatrixTCramerInv::Inv2x2<Element>(*this,det);
         return *this;
      }
      case 3:
      {
         TMatrixTCramerInv::Inv3x3<Element>(*this,det);
         return *this;
      }
      case 4:
      {
         TMatrixTCramerInv::Inv4x4<Element>(*this,det);
         return *this;
      }
      case 5:
      {
         TMatrixTCramerInv::Inv5x5<Element>(*this,det);
         return *this;
      }
      case 6:
      {
         TMatrixTCramerInv::Inv6x6<Element>(*this,det);
         return *this;
      }

      default:
      {
         if(typeid(Element) == typeid(Double_t))
            TDecompLU::InvertLU(*dynamic_cast<TMatrixD *>(this),Double_t(this->fTol),det);
         else {
            TMatrixD tmp(*this);
            TDecompLU::InvertLU(tmp,Double_t(this->fTol),det);
            const Double_t *p1 = tmp.GetMatrixArray();
                  Element  *p2 = this->GetMatrixArray();
            for (Int_t i = 0; i < this->GetNoElements(); i++)
               p2[i] = p1[i];
         }
         return *this;
      }
    }
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::Transpose(const TMatrixT<Element> &source)
{
  // Transpose a matrix.

   R__ASSERT(this->IsValid());
   R__ASSERT(source.IsValid());

   if (this == &source) {
      Element *ap = this->GetMatrixArray();
      if (this->fNrows == this->fNcols && this->fRowLwb == this->fColLwb) {
         for (Int_t i = 0; i < this->fNrows; i++) {
            const Int_t off_i = i*this->fNrows;
            for (Int_t j = i+1; j < this->fNcols; j++) {
               const Int_t off_j = j*this->fNcols;
               const Element tmp = ap[off_i+j];
               ap[off_i+j] = ap[off_j+i];
               ap[off_j+i] = tmp;
            }
         }
      } else {
         Element *oldElems = new Element[source.GetNoElements()];
         memcpy(oldElems,source.GetMatrixArray(),source.GetNoElements()*sizeof(Element));
         const Int_t nrows_old  = this->fNrows;
         const Int_t ncols_old  = this->fNcols;
         const Int_t rowlwb_old = this->fRowLwb;
         const Int_t collwb_old = this->fColLwb;

         this->fNrows  = ncols_old;  this->fNcols  = nrows_old;
         this->fRowLwb = collwb_old; this->fColLwb = rowlwb_old;
         for (Int_t irow = this->fRowLwb; irow < this->fRowLwb+this->fNrows; irow++) {
            for (Int_t icol = this->fColLwb; icol < this->fColLwb+this->fNcols; icol++) {
               const Int_t off = (icol-collwb_old)*ncols_old;
               (*this)(irow,icol) = oldElems[off+irow-rowlwb_old];
            }
         }
         delete [] oldElems;
      }
   } else {
      if (this->fNrows  != source.GetNcols()  || this->fNcols  != source.GetNrows() ||
          this->fRowLwb != source.GetColLwb() || this->fColLwb != source.GetRowLwb())
      {
         Error("Transpose","matrix has wrong shape");
         this->Invalidate();
         return *this;
      }

      const Element *sp1 = source.GetMatrixArray();
      const Element *scp = sp1; // Row source pointer
            Element *tp  = this->GetMatrixArray();
      const Element * const tp_last = this->GetMatrixArray()+this->fNelems;

      // (This: target) matrix is traversed row-wise way,
      // whilst the source matrix is scanned column-wise
      while (tp < tp_last) {
         const Element *sp2 = scp++;

         // Move tp to the next elem in the row and sp to the next elem in the curr col
         while (sp2 < sp1+this->fNelems) {
            *tp++ = *sp2;
            sp2 += this->fNrows;
         }
      }
      R__ASSERT(tp == tp_last && scp == sp1+this->fNrows);
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::Rank1Update(const TVectorT<Element> &v,Element alpha)
{
  // Perform a rank 1 operation on the matrix:
  //     A += alpha * v * v^T

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(v.IsValid());
      if (v.GetNoElements() < TMath::Max(this->fNrows,this->fNcols)) {
         Error("Rank1Update","vector too short");
         this->Invalidate();
         return *this;
      }
   }

   const Element * const pv = v.GetMatrixArray();
         Element *mp = this->GetMatrixArray();

   for (Int_t i = 0; i < this->fNrows; i++) {
      const Element tmp = alpha*pv[i];
      for (Int_t j = 0; j < this->fNcols; j++)
         *mp++ += tmp*pv[j];
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::Rank1Update(const TVectorT<Element> &v1,const TVectorT<Element> &v2,Element alpha)
{
  // Perform a rank 1 operation on the matrix:
  //     A += alpha * v1 * v2^T

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(v1.IsValid());
      R__ASSERT(v2.IsValid());
      if (v1.GetNoElements() < this->fNrows) {
         Error("Rank1Update","vector v1 too short");
         this->Invalidate();
         return *this;
      }

      if (v2.GetNoElements() < this->fNcols) {
         Error("Rank1Update","vector v2 too short");
         this->Invalidate();
         return *this;
      }
   }

   const Element * const pv1 = v1.GetMatrixArray();
   const Element * const pv2 = v2.GetMatrixArray();
         Element *mp = this->GetMatrixArray();

   for (Int_t i = 0; i < this->fNrows; i++) {
      const Element tmp = alpha*pv1[i];
      for (Int_t j = 0; j < this->fNcols; j++)
         *mp++ += tmp*pv2[j];
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
Element TMatrixT<Element>::Similarity(const TVectorT<Element> &v) const
{
// Calculate scalar v * (*this) * v^T

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(v.IsValid());
      if (this->fNcols != this->fNrows || this->fColLwb != this->fRowLwb) {
         Error("Similarity(const TVectorT &)","matrix is not square");
         return -1.;
      }

      if (this->fNcols != v.GetNrows() || this->fColLwb != v.GetLwb()) {
         Error("Similarity(const TVectorT &)","vector and matrix incompatible");
         return -1.;
      }
   }

   const Element *mp = this->GetMatrixArray(); // Matrix row ptr
   const Element *vp = v.GetMatrixArray();     // vector ptr

   Element sum1 = 0;
   const Element * const vp_first = vp;
   const Element * const vp_last  = vp+v.GetNrows();
   while (vp < vp_last) {
      Element sum2 = 0;
      for (const Element *sp = vp_first; sp < vp_last; )
         sum2 += *mp++ * *sp++;
      sum1 += sum2 * *vp++;
   }

   R__ASSERT(mp == this->GetMatrixArray()+this->GetNoElements());

   return sum1;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::NormByColumn(const TVectorT<Element> &v,Option_t *option)
{
  // Multiply/divide matrix columns by a vector:
  // option:
  // "D"   :  b(i,j) = a(i,j)/v(i)   i = 0,fNrows-1 (default)
  // else  :  b(i,j) = a(i,j)*v(i)

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(v.IsValid());
      if (v.GetNoElements() < this->fNrows) {
         Error("NormByColumn","vector shorter than matrix column");
         this->Invalidate();
         return *this;
      }
   }

   TString opt(option);
   opt.ToUpper();
   const Int_t divide = (opt.Contains("D")) ? 1 : 0;

   const Element *pv = v.GetMatrixArray();
         Element *mp = this->GetMatrixArray();
   const Element * const mp_last = mp+this->fNelems;

   if (divide) {
      for ( ; mp < mp_last; pv++) {
         for (Int_t j = 0; j < this->fNcols; j++)
         {
            R__ASSERT(*pv != 0.0);
            *mp++ /= *pv;
         }
      }
   } else {
      for ( ; mp < mp_last; pv++)
         for (Int_t j = 0; j < this->fNcols; j++)
            *mp++ *= *pv;
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::NormByRow(const TVectorT<Element> &v,Option_t *option)
{
  // Multiply/divide matrix rows with a vector:
  // option:
  // "D"   :  b(i,j) = a(i,j)/v(j)   i = 0,fNcols-1 (default)
  // else  :  b(i,j) = a(i,j)*v(j)

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(v.IsValid());
      if (v.GetNoElements() < this->fNcols) {
         Error("NormByRow","vector shorter than matrix column");
         this->Invalidate();
         return *this;
      }
   }

   TString opt(option);
   opt.ToUpper();
   const Int_t divide = (opt.Contains("D")) ? 1 : 0;

   const Element *pv0 = v.GetMatrixArray();
   const Element *pv  = pv0;
         Element *mp  = this->GetMatrixArray();
   const Element * const mp_last = mp+this->fNelems;

   if (divide) {
      for ( ; mp < mp_last; pv = pv0 )
         for (Int_t j = 0; j < this->fNcols; j++) {
            R__ASSERT(*pv != 0.0);
            *mp++ /= *pv++;
         }
    } else {
       for ( ; mp < mp_last; pv = pv0 )
          for (Int_t j = 0; j < this->fNcols; j++)
             *mp++ *= *pv++;
    }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator=(const TMatrixT<Element> &source)
{
   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator=(const TMatrixT &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   if (this != &source) {
      TObject::operator=(source);
      memcpy(fElements,source.GetMatrixArray(),this->fNelems*sizeof(Element));
      this->fTol = source.GetTol();
   }
   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator=(const TMatrixTSym<Element> &source)
{
   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator=(const TMatrixTSym &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   if ((TMatrixTBase<Element> *)this != (TMatrixTBase<Element> *)&source) {
      TObject::operator=(source);
      memcpy(fElements,source.GetMatrixArray(),this->fNelems*sizeof(Element));
      this->fTol = source.GetTol();
   }
   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator=(const TMatrixTSparse<Element> &source)
{
   if (gMatrixCheck &&
        this->GetNrows()  != source.GetNrows()  || this->GetNcols()  != source.GetNcols() ||
        this->GetRowLwb() != source.GetRowLwb() || this->GetColLwb() != source.GetColLwb()) {
      Error("operator=(const TMatrixTSparse &","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   if ((TMatrixTBase<Element> *)this != (TMatrixTBase<Element> *)&source) {
      TObject::operator=(source);
      memset(fElements,0,this->fNelems*sizeof(Element));

      const Element * const sp = source.GetMatrixArray();
            Element *       tp = this->GetMatrixArray();

      const Int_t * const pRowIndex = source.GetRowIndexArray();
      const Int_t * const pColIndex = source.GetColIndexArray();

      for (Int_t irow = 0; irow < this->fNrows; irow++ ) {
         const Int_t off = irow*this->fNcols;
         const Int_t sIndex = pRowIndex[irow];
         const Int_t eIndex = pRowIndex[irow+1];
         for (Int_t index = sIndex; index < eIndex; index++)
            tp[off+pColIndex[index]] = sp[index];
      }
      this->fTol = source.GetTol();
   }
   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator=(const TMatrixTLazy<Element> &lazy_constructor)
{
   R__ASSERT(this->IsValid());

   if (lazy_constructor.GetRowUpb() != this->GetRowUpb() ||
       lazy_constructor.GetColUpb() != this->GetColUpb() ||
       lazy_constructor.GetRowLwb() != this->GetRowLwb() ||
       lazy_constructor.GetColLwb() != this->GetColLwb()) {
      Error("operator=(const TMatrixTLazy&)", "matrix is incompatible with "
            "the assigned Lazy matrix");
      this->Invalidate();
      return *this;
   }

   lazy_constructor.FillIn(*this);
   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator=(Element val)
{
  // Assign val to every element of the matrix.

   R__ASSERT(this->IsValid());

   Element *ep = this->GetMatrixArray();
   const Element * const ep_last = ep+this->fNelems;
   while (ep < ep_last)
      *ep++ = val;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator+=(Element val)
{
  // Add val to every element of the matrix.

   R__ASSERT(this->IsValid());

   Element *ep = this->GetMatrixArray();
   const Element * const ep_last = ep+this->fNelems;
   while (ep < ep_last)
      *ep++ += val;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator-=(Element val)
{
  // Subtract val from every element of the matrix.

   R__ASSERT(this->IsValid());

   Element *ep = this->GetMatrixArray();
   const Element * const ep_last = ep+this->fNelems;
   while (ep < ep_last)
      *ep++ -= val;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(Element val)
{
  // Multiply every element of the matrix with val.

   R__ASSERT(this->IsValid());

   Element *ep = this->GetMatrixArray();
   const Element * const ep_last = ep+this->fNelems;
   while (ep < ep_last)
      *ep++ *= val;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator+=(const TMatrixT<Element> &source)
{
  // Add the source matrix.

   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator+=(const TMatrixT &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   const Element *sp = source.GetMatrixArray();
   Element *tp = this->GetMatrixArray();
   const Element * const tp_last = tp+this->fNelems;
   while (tp < tp_last)
      *tp++ += *sp++;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator+=(const TMatrixTSym<Element> &source)
{
  // Add the source matrix.

   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator+=(const TMatrixTSym &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   const Element *sp = source.GetMatrixArray();
   Element *tp = this->GetMatrixArray();
   const Element * const tp_last = tp+this->fNelems;
   while (tp < tp_last)
      *tp++ += *sp++;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator-=(const TMatrixT<Element> &source)
{
  // Subtract the source matrix.

   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator=-(const TMatrixT &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   const Element *sp = source.GetMatrixArray();
   Element *tp = this->GetMatrixArray();
   const Element * const tp_last = tp+this->fNelems;
   while (tp < tp_last)
      *tp++ -= *sp++;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator-=(const TMatrixTSym<Element> &source)
{
  // Subtract the source matrix.

   if (gMatrixCheck && !AreCompatible(*this,source)) {
      Error("operator=-(const TMatrixTSym &)","matrices not compatible");
      this->Invalidate();
      return *this;
   }

   const Element *sp = source.GetMatrixArray();
   Element *tp = this->GetMatrixArray();
   const Element * const tp_last = tp+this->fNelems;
   while (tp < tp_last)
      *tp++ -= *sp++;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(const TMatrixT<Element> &source)
{
  // Compute target = target * source inplace. Strictly speaking, it can't be
  // done inplace, though only the row of the target matrix needs to be saved.
  // "Inplace" multiplication is only allowed when the 'source' matrix is square.

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(source.IsValid());
      if (this->fNcols != source.GetNrows() || this->fColLwb != source.GetRowLwb() ||
          this->fNcols != source.GetNcols() || this->fColLwb != source.GetColLwb()) {
         Error("operator*=(const TMatrixT &)","source matrix has wrong shape");
         this->Invalidate();
         return *this;
      }
   }

   // Check for A *= A;
   const Element *sp;
   TMatrixT<Element> tmp;
   if (this == &source) {
      tmp.ResizeTo(source);
      tmp = source;
      sp = tmp.GetMatrixArray();
   }
   else
      sp = source.GetMatrixArray();

   // One row of the old_target matrix
   Element work[kWorkMax];
   Bool_t isAllocated = kFALSE;
   Element *trp = work;
   if (this->fNcols > kWorkMax) {
      isAllocated = kTRUE;
      trp = new Element[this->fNcols];
   }

         Element *cp   = this->GetMatrixArray();
   const Element *trp0 = cp; // Pointer to  target[i,0];
   const Element * const trp0_last = trp0+this->fNelems;
   while (trp0 < trp0_last) {
      memcpy(trp,trp0,this->fNcols*sizeof(Element));        // copy the i-th row of target, Start at target[i,0]
      for (const Element *scp = sp; scp < sp+this->fNcols; ) {  // Pointer to the j-th column of source,
                                                           // Start scp = source[0,0]
         Element cij = 0;
         for (Int_t j = 0; j < this->fNcols; j++) {
            cij += trp[j] * *scp;                        // the j-th col of source
            scp += this->fNcols;
         }
         *cp++ = cij;
         scp -= source.GetNoElements()-1;               // Set bcp to the (j+1)-th col
      }
      trp0 += this->fNcols;                            // Set trp0 to the (i+1)-th row
      R__ASSERT(trp0 == cp);
   }

   R__ASSERT(cp == trp0_last && trp0 == trp0_last);
   if (isAllocated)
      delete [] trp;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(const TMatrixTSym<Element> &source)
{
  // Compute target = target * source inplace. Strictly speaking, it can't be
  // done inplace, though only the row of the target matrix needs to be saved.

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(source.IsValid());
      if (this->fNcols != source.GetNrows() || this->fColLwb != source.GetRowLwb()) {
         Error("operator*=(const TMatrixTSym &)","source matrix has wrong shape");
         this->Invalidate();
         return *this;
      }
   }

   // Check for A *= A;
   const Element *sp;
   TMatrixT<Element> tmp;
   if ((TMatrixTBase<Element> *)this == (TMatrixTBase<Element> *)&source) {
      tmp.ResizeTo(source);
      tmp = source;
      sp = tmp.GetMatrixArray();
   }
   else
      sp = source.GetMatrixArray();

   // One row of the old_target matrix
   Element work[kWorkMax];
   Bool_t isAllocated = kFALSE;
   Element *trp = work;
   if (this->fNcols > kWorkMax) {
      isAllocated = kTRUE;
      trp = new Element[this->fNcols];
   }

         Element *cp   = this->GetMatrixArray();
   const Element *trp0 = cp; // Pointer to  target[i,0];
   const Element * const trp0_last = trp0+this->fNelems;
   while (trp0 < trp0_last) {
      memcpy(trp,trp0,this->fNcols*sizeof(Element));        // copy the i-th row of target, Start at target[i,0]
      for (const Element *scp = sp; scp < sp+this->fNcols; ) {  // Pointer to the j-th column of source,
                                                           // Start scp = source[0,0]
         Element cij = 0;
         for (Int_t j = 0; j < this->fNcols; j++) {
            cij += trp[j] * *scp;                        // the j-th col of source
            scp += this->fNcols;
         }
         *cp++ = cij;
         scp -= source.GetNoElements()-1;               // Set bcp to the (j+1)-th col
      }
      trp0 += this->fNcols;                            // Set trp0 to the (i+1)-th row
      R__ASSERT(trp0 == cp);
   } 

   R__ASSERT(cp == trp0_last && trp0 == trp0_last);
   if (isAllocated)
      delete [] trp;

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(const TMatrixTDiag_const<Element> &diag)
{
  // Multiply a matrix row by the diagonal of another matrix
  // matrix(i,j) *= diag(j), j=1,fNcols

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(diag.GetMatrix()->IsValid());
      R__ASSERT(this->fNcols == diag.GetNdiags());
      if (this->fNcols != diag.GetNdiags()) {
         Error("operator*=(const TMatrixTDiag_const &)","wrong diagonal length");
         this->Invalidate();
         return *this;
      }
   }

   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Int_t inc = diag.GetInc();
   while (mp < mp_last) {
      const Element *dp = diag.GetPtr();
      for (Int_t j = 0; j < this->fNcols; j++) {
         *mp++ *= *dp;
         dp += inc;
      }
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator/=(const TMatrixTDiag_const<Element> &diag)
{
  // Divide a matrix row by the diagonal of another matrix
  // matrix(i,j) /= diag(j)

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(diag.GetMatrix()->IsValid());
      if (this->fNcols != diag.GetNdiags()) {
         Error("operator/=(const TMatrixTDiag_const &)","wrong diagonal length");
         this->Invalidate();
         return *this;
      }
   }

   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Int_t inc = diag.GetInc();
   while (mp < mp_last) {
      const Element *dp = diag.GetPtr();
      for (Int_t j = 0; j < this->fNcols; j++) {
         R__ASSERT(*dp != 0.0);
         *mp++ /= *dp;
         dp += inc;
      }
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(const TMatrixTColumn_const<Element> &col)
{
  // Multiply a matrix by the column of another matrix
  // matrix(i,j) *= another(i,k) for fixed k

   const TMatrixTBase<Element> *mt = col.GetMatrix();

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(mt->IsValid());
      if (this->fNrows != mt->GetNrows()) {
         Error("operator*=(const TMatrixTColumn_const &)","wrong column length");
         this->Invalidate();
         return *this;
      }
   }

   const Element * const endp = col.GetPtr()+mt->GetNoElements();
   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Element *cp = col.GetPtr();      //  ptr
   const Int_t inc = col.GetInc();
   while (mp < mp_last) {
      R__ASSERT(cp < endp);
      for (Int_t j = 0; j < this->fNcols; j++)
         *mp++ *= *cp;
      cp += inc;
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator/=(const TMatrixTColumn_const<Element> &col)
{
  // Divide a matrix by the column of another matrix
  // matrix(i,j) /= another(i,k) for fixed k

   const TMatrixTBase<Element> *mt = col.GetMatrix();

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(mt->IsValid());
      if (this->fNrows != mt->GetNrows()) {
         Error("operator/=(const TMatrixTColumn_const &)","wrong column matrix");
         this->Invalidate();
         return *this;
      }
   }

   const Element * const endp = col.GetPtr()+mt->GetNoElements();
   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Element *cp = col.GetPtr();      //  ptr
   const Int_t inc = col.GetInc();
   while (mp < mp_last) {
      R__ASSERT(cp < endp);
      R__ASSERT(*cp != 0.0);
      for (Int_t j = 0; j < this->fNcols; j++)
         *mp++ /= *cp;
      cp += inc;
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator*=(const TMatrixTRow_const<Element> &row)
{
  // Multiply a matrix by the row of another matrix
  // matrix(i,j) *= another(k,j) for fixed k

   const TMatrixTBase<Element> *mt = row.GetMatrix();

   if (gMatrixCheck) {
      R__ASSERT(this->IsValid());
      R__ASSERT(mt->IsValid());
      if (this->fNcols != mt->GetNcols()) {
         Error("operator*=(const TMatrixTRow_const &)","wrong row length");
         this->Invalidate();
         return *this;
      }
   }

   const Element * const endp = row.GetPtr()+mt->GetNoElements();
   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Int_t inc = row.GetInc();
   while (mp < mp_last) {
      const Element *rp = row.GetPtr();    // Row ptr
      for (Int_t j = 0; j < this->fNcols; j++) {
         R__ASSERT(rp < endp);
         *mp++ *= *rp;
         rp += inc;
      }
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &TMatrixT<Element>::operator/=(const TMatrixTRow_const<Element> &row)
{
  // Divide a matrix by the row of another matrix
  // matrix(i,j) /= another(k,j) for fixed k

   const TMatrixTBase<Element> *mt = row.GetMatrix();
   R__ASSERT(this->IsValid());
   R__ASSERT(mt->IsValid());
 
   if (this->fNcols != mt->GetNcols()) {
      Error("operator/=(const TMatrixTRow_const &)","wrong row length");
      this->Invalidate();
      return *this;
   }

   const Element * const endp = row.GetPtr()+mt->GetNoElements();
   Element *mp = this->GetMatrixArray();  // Matrix ptr
   const Element * const mp_last = mp+this->fNelems;
   const Int_t inc = row.GetInc();
   while (mp < mp_last) {
      const Element *rp = row.GetPtr();    // Row ptr
      for (Int_t j = 0; j < this->fNcols; j++) {
         R__ASSERT(rp < endp);
         R__ASSERT(*rp != 0.0);
         *mp++ /= *rp;
         rp += inc;
      }
   }

   return *this;
}

//______________________________________________________________________________
template<class Element>
const TMatrixT<Element> TMatrixT<Element>::EigenVectors(TVectorT<Element> &eigenValues) const
{
  // Return a matrix containing the eigen-vectors ordered by descending values
  // of Re^2+Im^2 of the complex eigen-values .
  // If the matrix is asymmetric, only the real part of the eigen-values is
  // returned . For full functionality use TMatrixDEigen .

   if (!this->IsSymmetric())
      Warning("EigenVectors(TVectorT &)","Only real part of eigen-values will be returned");
   TMatrixDEigen eigen(*this);
   eigenValues.ResizeTo(this->fNrows);
   eigenValues = eigen.GetEigenValuesRe();
   return eigen.GetEigenVectors();
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator+(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
   TMatrixT<Element> target(source1);
   target += source2;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator+(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
   TMatrixT<Element> target(source1);
   target += source2;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator+(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
   return operator+(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator+(const TMatrixT<Element> &source,Element val)
{
   TMatrixT<Element> target(source);
   target += val;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator+(Element val,const TMatrixT<Element> &source)
{
   return operator+(source,val);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator-(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
   TMatrixT<Element> target(source1);
   target -= source2;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator-(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
   TMatrixT<Element> target(source1);
   target -= source2;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator-(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
   return Element(-1.0)*(operator-(source2,source1));
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator-(const TMatrixT<Element> &source,Element val)
{
   TMatrixT<Element> target(source);
   target -= val;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator-(Element val,const TMatrixT<Element> &source)
{
   return Element(-1.0)*operator-(source,val);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(Element val,const TMatrixT<Element> &source)
{
   TMatrixT<Element> target(source);
   target *= val;
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(const TMatrixT<Element> &source,Element val)
{
   return operator*(val,source);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
   TMatrixT<Element> target(source1,TMatrixT<Element>::kMult,source2);
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
   TMatrixT<Element> target(source1,TMatrixT<Element>::kMult,source2);
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
   TMatrixT<Element> target(source1,TMatrixT<Element>::kMult,source2);
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator*(const TMatrixTSym<Element> &source1,const TMatrixTSym<Element> &source2)
{
   TMatrixT<Element> target(source1,TMatrixT<Element>::kMult,source2);
   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator&&(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // Logical AND

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator&&(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last)
      *tp++ = (*sp1++ != 0.0 && *sp2++ != 0.0);

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator&&(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // Logical AND

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator&&(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last)
      *tp++ = (*sp1++ != 0.0 && *sp2++ != 0.0);

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator&&(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // Logical AND
   return operator&&(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator||(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // Logical OR

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator||(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last)
      *tp++ = (*sp1++ != 0.0 || *sp2++ != 0.0);

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator||(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // Logical OR

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator||(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last)
      *tp++ = (*sp1++ != 0.0 || *sp2++ != 0.0);

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator||(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // Logical OR
   return operator||(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 > source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator|(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) > (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 > source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator>(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) > (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 > source2
   return operator<=(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>=(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 >= source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator>=(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) >= (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>=(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 >= source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator>=(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) >= (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator>=(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 >= source2
   return operator<(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<=(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 <= source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator<=(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) <= (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<=(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 <= source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator<=(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) <= (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<=(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 <= source2
   return operator>(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 < source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator<(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) < (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 < source2

  TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator<(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp < tp_last) {
      *tp++ = (*sp1) < (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator<(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 < source2
   return operator>=(source2,source1);
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator!=(const TMatrixT<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 != source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator!=(const TMatrixT&,const TMatrixT&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp != tp_last) {
      *tp++ = (*sp1) != (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator!=(const TMatrixT<Element> &source1,const TMatrixTSym<Element> &source2)
{
  // source1 != source2

   TMatrixT<Element> target;

   if (gMatrixCheck && !AreCompatible(source1,source2)) {
      Error("operator!=(const TMatrixT&,const TMatrixTSym&)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   target.ResizeTo(source1);

   const Element *sp1 = source1.GetMatrixArray();
   const Element *sp2 = source2.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp != tp_last) {
      *tp++ = (*sp1) != (*sp2); sp1++; sp2++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator!=(const TMatrixTSym<Element> &source1,const TMatrixT<Element> &source2)
{
  // source1 != source2
   return operator!=(source2,source1);
}

/*
//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator!=(const TMatrixT<Element> &source1,Element val)
{
  // source1 != val

   TMatrixT<Element> target; target.ResizeTo(source1);

   const Element *sp = source1.GetMatrixArray();
         Element *tp = target.GetMatrixArray();
   const Element * const tp_last = tp+target.GetNoElements();
   while (tp != tp_last) {
      *tp++ = (*sp != val); sp++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> operator!=(Element val,const TMatrixT<Element> &source1)
{
  // val != source1
   return operator!=(source1,val);
}
*/

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &Add(TMatrixT<Element> &target,Element scalar,const TMatrixT<Element> &source)
{
  // Modify addition: target += scalar * source.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("Add(TMatrixT &,Element,const TMatrixT &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   if (scalar == 0) {
       while ( tp < ftp )
          *tp++  = scalar * (*sp++);
   } else if (scalar == 1.) {
       while ( tp < ftp )
          *tp++ = (*sp++);
   } else {
       while ( tp < ftp )
          *tp++ += scalar * (*sp++);
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &Add(TMatrixT<Element> &target,Element scalar,const TMatrixTSym<Element> &source)
{
  // Modify addition: target += scalar * source.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("Add(TMatrixT &,Element,const TMatrixTSym &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   while ( tp < ftp )
      *tp++ += scalar * (*sp++);

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &ElementMult(TMatrixT<Element> &target,const TMatrixT<Element> &source)
{
  // Multiply target by the source, element-by-element.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("ElementMult(TMatrixT &,const TMatrixT &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   while ( tp < ftp )
      *tp++ *= *sp++;

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &ElementMult(TMatrixT<Element> &target,const TMatrixTSym<Element> &source)
{
  // Multiply target by the source, element-by-element.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("ElementMult(TMatrixT &,const TMatrixTSym &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   while ( tp < ftp )
      *tp++ *= *sp++;

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &ElementDiv(TMatrixT<Element> &target,const TMatrixT<Element> &source)
{
  // Divide target by the source, element-by-element.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("ElementDiv(TMatrixT &,const TMatrixT &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   while ( tp < ftp ) {
      R__ASSERT(*sp != 0.0);
      *tp++ /= *sp++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
TMatrixT<Element> &ElementDiv(TMatrixT<Element> &target,const TMatrixTSym<Element> &source)
{
  // Multiply target by the source, element-by-element.

   if (gMatrixCheck && !AreCompatible(target,source)) {
      ::Error("ElementDiv(TMatrixT &,const TMatrixTSym &)","matrices not compatible");
      target.Invalidate();
      return target;
   }

   const Element *sp  = source.GetMatrixArray();
         Element *tp  = target.GetMatrixArray();
   const Element *ftp = tp+target.GetNoElements();
   while ( tp < ftp ) {
      R__ASSERT(*sp != 0.0);
      *tp++ /= *sp++;
   }

   return target;
}

//______________________________________________________________________________
template<class Element>
void AMultB(const Element * const ap,Int_t na,Int_t ncolsa,
            const Element * const bp,Int_t nb,Int_t ncolsb,Element *cp)
{
   const Element *arp0 = ap;                     // Pointer to  A[i,0];
   while (arp0 < ap+na) {
      for (const Element *bcp = bp; bcp < bp+ncolsb; ) { // Pointer to the j-th column of B, Start bcp = B[0,0]
         const Element *arp = arp0;                       // Pointer to the i-th row of A, reset to A[i,0]
         Element cij = 0;
         while (bcp < bp+nb) {                     // Scan the i-th row of A and
            cij += *arp++ * *bcp;                   // the j-th col of B
            bcp += ncolsb;
         }
         *cp++ = cij;
         bcp -= nb-1;                              // Set bcp to the (j+1)-th col
      }
      arp0 += ncolsa;                             // Set ap to the (i+1)-th row
   }
}

//______________________________________________________________________________
template<class Element>
void AtMultB(const Element * const ap,Int_t ncolsa,
             const Element * const bp,Int_t nb,Int_t ncolsb,Element *cp)
{
   const Element *acp0 = ap;           // Pointer to  A[i,0];
   while (acp0 < ap+ncolsa) {
      for (const Element *bcp = bp; bcp < bp+ncolsb; ) { // Pointer to the j-th column of B, Start bcp = B[0,0]
         const Element *acp = acp0;                       // Pointer to the i-th column of A, reset to A[0,i]
         Element cij = 0;
         while (bcp < bp+nb) {           // Scan the i-th column of A and
            cij += *acp * *bcp;           // the j-th col of B
            acp += ncolsa;
            bcp += ncolsb;
         }
         *cp++ = cij;
         bcp -= nb-1;                    // Set bcp to the (j+1)-th col
      }
      acp0++;                           // Set acp0 to the (i+1)-th col
   }
}

//______________________________________________________________________________
template<class Element>
void AMultBt(const Element * const ap,Int_t na,Int_t ncolsa,
             const Element * const bp,Int_t nb,Int_t ncolsb,Element *cp)
{
   const Element *arp0 = ap;                    // Pointer to  A[i,0];
   while (arp0 < ap+na) {
      const Element *brp0 = bp;                  // Pointer to  B[j,0];
      while (brp0 < bp+nb) {
         const Element *arp = arp0;               // Pointer to the i-th row of A, reset to A[i,0]
         const Element *brp = brp0;               // Pointer to the j-th row of B, reset to B[j,0]
         Element cij = 0;
         while (brp < brp0+ncolsb)                 // Scan the i-th row of A and
            cij += *arp++ * *brp++;                 // the j-th row of B
         *cp++ = cij;
         brp0 += ncolsb;                           // Set brp0 to the (j+1)-th row
      }
      arp0 += ncolsa;                             // Set arp0 to the (i+1)-th row
   }
}

//______________________________________________________________________________
template<class Element>
void TMatrixT<Element>::Streamer(TBuffer &R__b)
{
  // Stream an object of class TMatrixT.

   if (R__b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         Clear();
         TMatrixT<Element>::Class()->ReadBuffer(R__b,this,R__v,R__s,R__c);
      } else if (R__v == 2) { //process old version 2
         Clear();
         TObject::Streamer(R__b);
         this->MakeValid();
         R__b >> this->fNrows;
         R__b >> this->fNcols;
         R__b >> this->fNelems;
         R__b >> this->fRowLwb;
         R__b >> this->fColLwb;
         Char_t isArray;
         R__b >> isArray;
         if (isArray) {
            if (this->fNelems > 0) {
               fElements = new Element[this->fNelems];
               R__b.ReadFastArray(fElements,this->fNelems);
            } else
               fElements = 0;
         }
         R__b.CheckByteCount(R__s,R__c,TMatrixT<Element>::IsA());
      } else { //====process old versions before automatic schema evolution
         TObject::Streamer(R__b);
         this->MakeValid();
         R__b >> this->fNrows;
         R__b >> this->fNcols;
         R__b >> this->fRowLwb;
         R__b >> this->fColLwb;
         this->fNelems = R__b.ReadArray(fElements);
         R__b.CheckByteCount(R__s,R__c,TMatrixT<Element>::IsA());
      }
      // in version <=2 , the matrix was stored column-wise
      if (R__v <= 2) {
         for (Int_t i = 0; i < this->fNrows; i++) {
            const Int_t off_i = i*this->fNcols;
            for (Int_t j = i; j < this->fNcols; j++) {
               const Int_t off_j = j*this->fNrows;
               const Element tmp = fElements[off_i+j];
               fElements[off_i+j] = fElements[off_j+i];
               fElements[off_j+i] = tmp;
            }
         }
      }
      if (this->fNelems > 0 && this->fNelems <= this->kSizeMax) {
         memcpy(fDataStack,fElements,this->fNelems*sizeof(Element));
         delete [] fElements;
         fElements = fDataStack;
      } else if (this->fNelems < 0)
         this->Invalidate();
      } else {
         TMatrixT<Element>::Class()->WriteBuffer(R__b,this);
   }
}

template class TMatrixT<Float_t>;

#ifndef ROOT_TMatrixFfwd
#include "TMatrixFfwd.h"
#endif
#ifndef ROOT_TMatrixFSymfwd
#include "TMatrixFSymfwd.h"
#endif

template TMatrixF  operator+  <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator+  <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator+  <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator+  <Float_t>(const TMatrixF    &source ,      Float_t      val    );
template TMatrixF  operator+  <Float_t>(      Float_t      val    ,const TMatrixF    &source );
template TMatrixF  operator-  <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator-  <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator-  <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator-  <Float_t>(const TMatrixF    &source ,      Float_t      val    );
template TMatrixF  operator-  <Float_t>(      Float_t      val    ,const TMatrixF    &source );
template TMatrixF  operator*  <Float_t>(      Float_t      val    ,const TMatrixF    &source );
template TMatrixF  operator*  <Float_t>(const TMatrixF    &source ,      Float_t      val    );
template TMatrixF  operator*  <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator*  <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator*  <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator*  <Float_t>(const TMatrixFSym &source1,const TMatrixFSym &source2);
template TMatrixF  operator&& <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator&& <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator&& <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator|| <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator|| <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator|| <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator>  <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator>  <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator>  <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator>= <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator>= <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator>= <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator<= <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator<= <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator<= <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator<  <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator<  <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator<  <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);
template TMatrixF  operator!= <Float_t>(const TMatrixF    &source1,const TMatrixF    &source2);
template TMatrixF  operator!= <Float_t>(const TMatrixF    &source1,const TMatrixFSym &source2);
template TMatrixF  operator!= <Float_t>(const TMatrixFSym &source1,const TMatrixF    &source2);

template TMatrixF &Add        <Float_t>(TMatrixF &target,      Float_t      scalar,const TMatrixF    &source);
template TMatrixF &Add        <Float_t>(TMatrixF &target,      Float_t      scalar,const TMatrixFSym &source);
template TMatrixF &ElementMult<Float_t>(TMatrixF &target,const TMatrixF    &source);
template TMatrixF &ElementMult<Float_t>(TMatrixF &target,const TMatrixFSym &source);
template TMatrixF &ElementDiv <Float_t>(TMatrixF &target,const TMatrixF    &source);
template TMatrixF &ElementDiv <Float_t>(TMatrixF &target,const TMatrixFSym &source);

template void AMultB <Float_t>(const Float_t * const ap,Int_t na,Int_t ncolsa,
                               const Float_t * const bp,Int_t nb,Int_t ncolsb,Float_t *cp);
template void AtMultB<Float_t>(const Float_t * const ap,Int_t ncolsa,
                               const Float_t * const bp,Int_t nb,Int_t ncolsb,Float_t *cp);
template void AMultBt<Float_t>(const Float_t * const ap,Int_t na,Int_t ncolsa,
                               const Float_t * const bp,Int_t nb,Int_t ncolsb,Float_t *cp);

#ifndef ROOT_TMatrixDfwd
#include "TMatrixDfwd.h"
#endif
#ifndef ROOT_TMatrixDSymfwd
#include "TMatrixDSymfwd.h"
#endif

template class TMatrixT<Double_t>;

template TMatrixD  operator+  <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator+  <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator+  <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator+  <Double_t>(const TMatrixD    &source ,      Double_t     val    );
template TMatrixD  operator+  <Double_t>(      Double_t     val    ,const TMatrixD    &source );
template TMatrixD  operator-  <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator-  <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator-  <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator-  <Double_t>(const TMatrixD    &source ,      Double_t     val    );
template TMatrixD  operator-  <Double_t>(      Double_t     val    ,const TMatrixD    &source );
template TMatrixD  operator*  <Double_t>(      Double_t     val    ,const TMatrixD    &source );
template TMatrixD  operator*  <Double_t>(const TMatrixD    &source ,      Double_t     val    );
template TMatrixD  operator*  <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator*  <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator*  <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator*  <Double_t>(const TMatrixDSym &source1,const TMatrixDSym &source2);
template TMatrixD  operator&& <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator&& <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator&& <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator|| <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator|| <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator|| <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator>  <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator>  <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator>  <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator>= <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator>= <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator>= <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator<= <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator<= <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator<= <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator<  <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator<  <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator<  <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);
template TMatrixD  operator!= <Double_t>(const TMatrixD    &source1,const TMatrixD    &source2);
template TMatrixD  operator!= <Double_t>(const TMatrixD    &source1,const TMatrixDSym &source2);
template TMatrixD  operator!= <Double_t>(const TMatrixDSym &source1,const TMatrixD    &source2);

template TMatrixD &Add        <Double_t>(TMatrixD &target,      Double_t     scalar,const TMatrixD    &source);
template TMatrixD &Add        <Double_t>(TMatrixD &target,      Double_t     scalar,const TMatrixDSym &source);
template TMatrixD &ElementMult<Double_t>(TMatrixD &target,const TMatrixD    &source);
template TMatrixD &ElementMult<Double_t>(TMatrixD &target,const TMatrixDSym &source);
template TMatrixD &ElementDiv <Double_t>(TMatrixD &target,const TMatrixD    &source);
template TMatrixD &ElementDiv <Double_t>(TMatrixD &target,const TMatrixDSym &source);

template void AMultB <Double_t>(const Double_t * const ap,Int_t na,Int_t ncolsa,
                                const Double_t * const bp,Int_t nb,Int_t ncolsb,Double_t *cp);
template void AtMultB<Double_t>(const Double_t * const ap,Int_t ncolsa,
                                const Double_t * const bp,Int_t nb,Int_t ncolsb,Double_t *cp);
template void AMultBt<Double_t>(const Double_t * const ap,Int_t na,Int_t ncolsa,
                                const Double_t * const bp,Int_t nb,Int_t ncolsb,Double_t *cp);
