%{

@T begin

import mt.base

record Person
  name: string
  age: double
end

end

%}

% @T :: [Person] = (?, ?)
function [person] = make_person(name, age)

% @T constructor
person = struct( 'name', name, 'age', age );

end