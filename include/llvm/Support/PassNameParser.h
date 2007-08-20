//===- llvm/Support/PassNameParser.h ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file the PassNameParser and FilteredPassNameParser<> classes, which are
// used to add command line arguments to a utility for all of the passes that
// have been registered into the system.
//
// The PassNameParser class adds ALL passes linked into the system (that are
// creatable) as command line arguments to the tool (when instantiated with the
// appropriate command line option template).  The FilteredPassNameParser<>
// template is used for the same purposes as PassNameParser, except that it only
// includes passes that have a PassType that are compatible with the filter
// (which is the template argument).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_PASS_NAME_PARSER_H
#define LLVM_SUPPORT_PASS_NAME_PARSER_H

#include "llvm/Support/CommandLine.h"
#include "llvm/Pass.h"
#include <algorithm>
#include <cstring>

namespace llvm {

//===----------------------------------------------------------------------===//
// PassNameParser class - Make use of the pass registration mechanism to
// automatically add a command line argument to opt for each pass.
//
class PassNameParser : public PassRegistrationListener,
                       public cl::parser<const PassInfo*> {
  cl::Option *Opt;
public:
  PassNameParser() : Opt(0) {}

  void initialize(cl::Option &O) {
    Opt = &O;
    cl::parser<const PassInfo*>::initialize(O);

    // Add all of the passes to the map that got initialized before 'this' did.
    enumeratePasses();
  }

  // ignorablePassImpl - Can be overriden in subclasses to refine the list of
  // which passes we want to include.
  //
  virtual bool ignorablePassImpl(const PassInfo *P) const { return false; }

  inline bool ignorablePass(const PassInfo *P) const {
    // Ignore non-selectable and non-constructible passes!  Ignore
    // non-optimizations.
    return P->getPassArgument() == 0 || *P->getPassArgument() == 0 ||
           P->getNormalCtor() == 0 || ignorablePassImpl(P);
  }

  // Implement the PassRegistrationListener callbacks used to populate our map
  //
  virtual void passRegistered(const PassInfo *P) {
    if (ignorablePass(P) || !Opt) return;
    if (findOption(P->getPassArgument()) != getNumOptions()) {
      cerr << "Two passes with the same argument (-"
           << P->getPassArgument() << ") attempted to be registered!\n";
      abort();
    }
    addLiteralOption(P->getPassArgument(), P, P->getPassName());
  }
  virtual void passEnumerate(const PassInfo *P) { passRegistered(P); }

  // ValLessThan - Provide a sorting comparator for Values elements...
  typedef std::pair<const char*,
                    std::pair<const PassInfo*, const char*> > ValType;
  static bool ValLessThan(const ValType &VT1, const ValType &VT2) {
    return std::string(VT1.first) < std::string(VT2.first);
  }

  // printOptionInfo - Print out information about this option.  Override the
  // default implementation to sort the table before we print...
  virtual void printOptionInfo(const cl::Option &O, unsigned GlobalWidth) const{
    PassNameParser *PNP = const_cast<PassNameParser*>(this);
    std::sort(PNP->Values.begin(), PNP->Values.end(), ValLessThan);
    cl::parser<const PassInfo*>::printOptionInfo(O, GlobalWidth);
  }
};

//===----------------------------------------------------------------------===//
// FilteredPassNameParser class - Make use of the pass registration
// mechanism to automatically add a command line argument to opt for
// each pass that satisfies a filter criteria.  Filter should return
// true for passes to be registered as command-line options.
//
template<typename Filter>
class FilteredPassNameParser : public PassNameParser {
private:
  Filter filter;

public:
  bool ignorablePassImpl(const PassInfo *P) const { return !filter(*P); }
};

//===----------------------------------------------------------------------===//
// PassArgFilter - A filter for use with PassNameFilterParser that only
// accepts a Pass whose Arg matches certain strings.
//
// Use like this:
//
// extern const char AllowedPassArgs[] = "-anders_aa -dse";
//
// static cl::list<
//   const PassInfo*,
//   bool,
//   FilteredPassNameParser<PassArgFilter<AllowedPassArgs> > >
// PassList(cl::desc("LLVM optimizations available:"));
//
// Only the -anders_aa and -dse options will be available to the user.
//
template<const char *Args>
class PassArgFilter {
public:
  bool operator()(const PassInfo &P) const {
    return(std::strstr(Args, P.getPassArgument()));
  }
};

} // End llvm namespace
#endif
