${"###"} ${method.name}

${method.description}

% if len(method.parameters) or len(method.returns):
${"####"} Parameters and Returns
| direction | name | type | description |
|:---------:|------|------|-------------|
    % for p in method.parameters:
| in | ${p.markdown(loader)} |
    % endfor
    % for r in method.returns:
| out | ${r.markdown(loader)} |
    % endfor
% endif

% if len(method.errors):
${"####"} Errors
    % for e in method.errors:
 * ${e}
    % endfor
% endif
