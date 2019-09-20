/// \file ROOT/RDrawable.hxx
/// \ingroup Base ROOT7
/// \author Axel Naumann <axel@cern.ch>
/// \date 2015-08-07
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RDrawable
#define ROOT7_RDrawable

#include <memory>
#include <string>
#include <vector>

#include <ROOT/RAttrValues.hxx>
#include <ROOT/RStyle.hxx>


namespace ROOT {
namespace Experimental {

class RMenuItems;
class RPadBase;
class RAttrBase;


namespace Internal {
class RPadPainter;

class RIOSharedBase {
public:
   virtual const void *GetIOPtr() const = 0;
   virtual bool HasShared() const = 0;
   virtual void *MakeShared() = 0;
   virtual void SetShared(void *shared) = 0;
   virtual ~RIOSharedBase() {}
};

using RIOSharedVector_t = std::vector<RIOSharedBase *>;

template<class T>
class RIOShared final : public RIOSharedBase {
   std::shared_ptr<T>  fShared;  ///<!   holder of object
   T* fIO{nullptr};              ///<    plain pointer for IO
public:
   const void *GetIOPtr() const final { return fIO; }
   virtual bool HasShared() const final { return fShared.get() != nullptr; }
   virtual void *MakeShared() final { fShared.reset(fIO); return &fShared; }
   virtual void SetShared(void *shared) final { fShared = *((std::shared_ptr<T> *) shared); }

   RIOShared() = default;

   RIOShared(const std::shared_ptr<T> &ptr) : RIOSharedBase()
   {
      fShared = ptr;
      fIO = ptr.get();
   }

   RIOShared &operator=(const std::shared_ptr<T> &ptr)
   {
      fShared = ptr;
      fIO = ptr.get();
      return *this;
   }

   operator bool() const { return !!fShared || !!fIO; }

   const T *get() const { return fShared ? fShared.get() : fIO; }
   T *get() { return fShared ? fShared.get() : fIO; }

   const T *operator->() const { return get(); }
   T *operator->() { return get(); }

   std::shared_ptr<T> get_shared() const { return fShared; }

   void reset() { fShared.reset(); fIO = nullptr; }
};

}

/** \class RDrawable
  Base class for drawable entities: objects that can be painted on a `RPad`.
 */

class RDrawable {

friend class RPadBase;
friend class RAttrBase;
friend class RStyle;

private:

   std::string  fId; ///< object identifier, unique inside RCanvas

   RAttrValues fAttr; ///< attributes values

   std::weak_ptr<RStyle> fStyle; ///<! style applied for RDrawable

   std::string fType;          ///<! drawable type, not stored in the root file, must be initialized in constructor

   std::string fUserClass;     ///<  user defined drawable class, can later go inside map


protected:

   virtual void CollectShared(Internal::RIOSharedVector_t &) {}
   RAttrValues *GetAttr() { return &fAttr; }

   bool MatchSelector(const std::string &selector) const;

public:

   explicit RDrawable(const std::string &type) : fType(type) {}

   virtual ~RDrawable();

   virtual void Paint(Internal::RPadPainter &onPad);

   /** Method can be used to provide menu items for the drawn object */
   virtual void PopulateMenu(RMenuItems &){};

   virtual void Execute(const std::string &);

   std::string GetId() const { return fId; }

   void UseStyle(const std::shared_ptr<RStyle> &style) { fStyle = style; }
   void ClearStyle() { fStyle.reset(); }

   void SetUserClass(const std::string &cl) { fUserClass = cl; }
   std::string GetUserClass() const { return fUserClass; }

   std::string GetType() const { return fType; }

};


} // namespace Experimental
} // namespace ROOT

#endif
