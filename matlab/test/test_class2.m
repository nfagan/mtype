y = 10;

function x()
y = y + 1;

x = @y.z;
another = @z;
another()

  function z()
    y = y + 2;
  end

z();
end