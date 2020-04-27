function(configure_type)

    set(sources
            components.hpp
            components.cpp
            debug.hpp
            error.hpp
            error.cpp
            instance.hpp
            instance.cpp
            library.hpp
            library.cpp
            destructured_member_visitor.hpp
            pending_external_functions.hpp
            pending_external_functions.cpp
            simplify.hpp
            simplify.cpp
            subscripts.hpp
            subscripts.cpp
            substitution.hpp
            substitution.cpp
            type.hpp
            type.cpp
            types.hpp
            types.cpp
            type_constraint_gen.hpp
            type_constraint_gen.cpp
            type_identifier_resolution.hpp
            type_identifier_resolution.cpp
            type_import_resolution.hpp
            type_import_resolution.cpp
            type_properties.hpp
            type_properties.cpp
            type_relation.hpp
            type_relation.cpp
            type_relationships.hpp
            type_relationships.cpp
            type_representation.hpp
            type_representation.cpp
            type_scope.hpp
            type_scope.cpp
            type_store.hpp
            type_store.cpp
            unification.hpp
            unification.cpp
            union_member_visitor.hpp
            union_member_visitor.cpp
            )

    foreach(source ${sources})
        target_sources(${PROJECT_NAME} PRIVATE ${MT_SOURCE_DIR}/type/${source})
    endforeach()

endfunction(configure_type)

configure_type()