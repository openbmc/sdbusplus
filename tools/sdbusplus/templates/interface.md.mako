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
% else:
No properties.
% endif

${"##"} Signals
% if len(interface.signals):
    % for s in interface.signals:
${s.markdown(loader)}
    %endfor
% else:
No signals.
% endif

${"##"} Enumerations
% if len(interface.enums):
    % for e in interface.enums:
${"###"} ${e.name}

${e.description}

| name | description |
|------|-------------|
        % for v in e.values:
| **${v.name}** | ${ v.description.strip() } |
        % endfor
    % endfor
% else:
No enumerations.
% endif
