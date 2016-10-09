# ${interface.name}

${interface.description}

${"##"} Methods
% if len(interface.methods):
    % for m in interface.methods:
${m.markdown(loader)}
    % endfor
% else:
No methods.
% endif

${"##"} Properties
% if len(interface.properties):
| name | type | description |
|------|------|-------------|
    % for p in interface.properties:
| ${p.markdown(loader)} |
    % endfor
%else:
No properties.
%endif
