# mtype

`mtype` presents a type language and type checker for MATLAB code.

Type annotations live in the comments of otherwise standard MATLAB code. Here's a short example:

my_typed_add.m:
```MATLAB
%  @T import mt.base
%  @T :: [double] = (double, double)
function c = my_typed_add(a, b)
c = a + b;
end
```

use_typed_add.m:
```MATLAB
function a = use_typed_add()

a = my_typed_add( 1, 2 );
b = my_typed_add( '', 2 ); % error. 

end
```

Run the type checker at the command line:
`mtype use_typed_add -pf /path/to/path/file.txt`

