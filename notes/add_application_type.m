%{

type_constraint_gen.cpp:
  function_call_expr: 
    const auto func_type = 
      type_store.make_abstraction(..., new_type_variable(), new_type_variable());
    
    const auto app_type = 
      type_store.make_application(func_type, args_type, result_type)

unification.cpp:
  check_push_function:
    if (is_concrete(app.args)) {
      const auto search_result = library.search_function(app.abstraction, app.args);

      push_type_equation(app.abstraction, search_result.source)
    }

  check_application:
    if (is_concrete(app.arguments) && is_concrete(app.abstraction.inputs) {
      //  Application arguments must be subtypes of abstraction arguments,
      //  and outputs must be supertypes of abstraction outputs
      push_type_equation(app.inputs, app.abstraction.inputs)
      push_type_equation(app.abstraction.outputs, app.outputs);
    }
    

%}