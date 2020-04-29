%{

@T begin
  begin
    % Unless explicitly exported, declarations are private to the defining
    % file.
    let component = uint8 | double
  end

  begin export
    % imports in a `begin export` block are re-exported.
    import mt.base

    record Color
      r: component
      g: component
      b: component
    end
  end
end

%}