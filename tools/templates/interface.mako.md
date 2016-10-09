# ${interface.name}

${interface.description}

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
