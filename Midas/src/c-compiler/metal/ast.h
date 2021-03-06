#ifndef VALE_AST_H_
#define VALE_AST_H_

#include "types.h"

#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Analysis.h>

#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

using std::move;

// Defined elsewhere
class Block;
class Expression;

// Defined in this file
class Program;
class StructDefinition;
class StructMember;
class InterfaceDefinition;
class Edge;
class Function;
class Prototype;
class Name;

class Name {
public:
  std::string name;

  Name(const std::string& name_) : name(name_) {}
};

class Program {
public:
    std::unordered_map<std::string, InterfaceDefinition*> interfaces;
    std::unordered_map<std::string, StructDefinition*> structs;
    std::unordered_map<std::string, KnownSizeArrayT*> knownSizeArrays;
    std::unordered_map<std::string, UnknownSizeArrayT*> unknownSizeArrays;
    // Get rid of this; since there's no IDs anymore we can have a stable
    // hardcoded NameH("__Pack", Some(List()), None, None).
    StructReferend* emptyTupleStructRef;
    std::unordered_map<std::string, Prototype*> externs;
    std::unordered_map<std::string, Function*> functions;
    std::unordered_map<Referend*, Prototype*> immDestructorsByKind;
    std::unordered_map<Name*, std::string> exportedNameByFullName;

    Program(
        std::unordered_map<std::string, InterfaceDefinition*> interfaces_,
        std::unordered_map<std::string, StructDefinition*> structs_,
        std::unordered_map<std::string, KnownSizeArrayT*> knownSizeArrays_,
        std::unordered_map<std::string, UnknownSizeArrayT*> unknownSizeArrays_,
        StructReferend* emptyTupleStructRef_,
        std::unordered_map<std::string, Prototype*> externs_,
        std::unordered_map<std::string, Function*> functions_,
        std::unordered_map<Referend*, Prototype*> immDestructorsByKind_,
        std::unordered_map<Name*, std::string> exportedNameByFullName_) :
      interfaces(move(interfaces_)),
      structs(move(structs_)),
      knownSizeArrays(move(knownSizeArrays_)),
      unknownSizeArrays(move(unknownSizeArrays_)),
      emptyTupleStructRef(emptyTupleStructRef_),
      externs(move(externs_)),
      functions(move(functions_)),
      immDestructorsByKind(move(immDestructorsByKind_)),
      exportedNameByFullName(move(exportedNameByFullName_)) {}


  StructDefinition* getStruct(Name* name) {
    auto iter = structs.find(name->name);
    assert(iter != structs.end());
    return iter->second;
  }
  InterfaceDefinition* getInterface(Name* name) {
    auto iter = interfaces.find(name->name);
    assert(iter != interfaces.end());
    return iter->second;
  }
  Prototype* getImmDestructor(Referend* referend) {
    auto iter = immDestructorsByKind.find(referend);
    assert(iter != immDestructorsByKind.end());
    return iter->second;
  }
  bool isExported(Name* name) {
    auto exportedNameI = exportedNameByFullName.find(name);
    return exportedNameI != exportedNameByFullName.end();
  }
  std::string getExportedName(Name* name) {
    auto exportedNameI = exportedNameByFullName.find(name);
    if (exportedNameI == exportedNameByFullName.end()) {
      std::cerr << "No exported name for " << name->name << std::endl;
      assert(false);
    }
    return exportedNameI->second;
  }
  Weakability getReferendWeakability(Referend* referend);
  Mutability getReferendMutability(Referend* referendM);
};

class InterfaceMethod {
public:
  Prototype* prototype;
  int virtualParamIndex;

  InterfaceMethod(
      Prototype* prototype_,
      int virtualParamIndex_) :
      prototype(prototype_),
      virtualParamIndex(virtualParamIndex_) {}
};

// Represents how a struct implements an interface.
// Each edge has a vtable.
class Edge {
public:
  StructReferend* structName;
  InterfaceReferend* interfaceName;
  std::vector<std::pair<InterfaceMethod*, Prototype*>> structPrototypesByInterfaceMethod;

  Edge(
      StructReferend* structName_,
      InterfaceReferend* interfaceName_,
      std::vector<std::pair<InterfaceMethod*, Prototype*>> structPrototypesByInterfaceMethod_) :
      structName(structName_),
      interfaceName(interfaceName_),
      structPrototypesByInterfaceMethod(structPrototypesByInterfaceMethod_) {}
};

class StructDefinition {
public:
    Name* name;
    StructReferend* referend;
    Mutability mutability;
    std::vector<Edge*> edges;
    std::vector<StructMember*> members;
    Weakability weakability;

    StructDefinition(
        Name* name_,
        StructReferend* referend_,
        Mutability mutability_,
        std::vector<Edge*> edges_,
        std::vector<StructMember*> members_,
        Weakability weakable_) :
        name(name_),
        referend(referend_),
        mutability(mutability_),
        edges(edges_),
        members(members_),
        weakability(weakable_) {}

    Edge* getEdgeForInterface(Name* interfaceName) {
      for (auto e : edges) {
        if (e->interfaceName->fullName == interfaceName)
          return e;
      }
      assert(false);
      return nullptr;
    }
};

class StructMember {
public:
    std::string name;
    Variability variability;
    Reference* type;

    StructMember(
        std::string name_,
        Variability variability_,
        Reference* type_) :
        name(name_),
        variability(variability_),
        type(type_) {}
};


class InterfaceDefinition {
public:
    Name* name;
    InterfaceReferend* referend;
    Mutability mutability;
    std::vector<Name*> superInterfaces;
    std::vector<InterfaceMethod*> methods;
    Weakability weakability;

    InterfaceDefinition(
        Name* name_,
        InterfaceReferend* referend_,
        Mutability mutability_,
        const std::vector<Name*>& superInterfaces_,
        const std::vector<InterfaceMethod*>& methods_,
        Weakability weakable_) :
      name(name_),
      referend(referend_),
      mutability(mutability_),
      superInterfaces(superInterfaces_),
      methods(methods_),
      weakability(weakable_) {
      assert((uint64_t)name > 0x10000);
      assert((uint64_t)referend > 0x10000);
    }
};

class Function {
public:
    Prototype* prototype;
    Expression* block;

    Function(

        Prototype* prototype_,
    Expression* block_
        ) :
        prototype(prototype_),
        block(block_) {}
};

// Interned
class Prototype {
public:
    Name* name;
    std::vector<Reference*> params;
    Reference* returnType;

    Prototype(
        Name* name_,
        std::vector<Reference*> params_,
        Reference* returnType_) :
      name(name_),
      params(std::move(params_)),
      returnType(returnType_) {}
};

#endif
