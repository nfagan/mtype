function myfunc
% local is an undefined variable
local(1); % Errors
local = 2; 
disp(local);
end

function local(x)
disp(x)
end