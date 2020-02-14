%{

//  in the AST
struct FunctionDefNode {
  FunctionDefHandle handle;
}

struct FunctionDefHandle {
  int64_t def_index;
}

struct FunctionReference {
  int64_t name;
  FunctionDefHandle def;
};

struct FunctionReferenceHandle {
  int64_t reference_index;
};

FunctionReference* -> FunctionReferenceHandle
FunctionDef* -> FunctionDefHandle

%}