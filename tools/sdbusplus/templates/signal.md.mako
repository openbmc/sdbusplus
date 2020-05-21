${"###"} ${signal.name}

${signal.description}

%if len(signal.properties):
${"####"} Properties
| name | type | description |
|------|------|-------------|
    % for p in signal.properties:
| ${p.markdown(loader)} |
    % endfor
% endif
