---
layout: default
---
# Documentation

Welcome !

{% if project and project.name %}
This is {{ project.name }}!
{% endif %}

{% if model.program %}

## Namespaces

{% assign ns_list = model.program.global_namespace.entities | filter_by_type: 'namespace' %}

- [global namespace](namespaces/global.md)
{% for ns in ns_list %}
- [{{ ns.name }}]({{ ns | get_url }})
{% endfor %}

{% assign classes = model.program.global_namespace.entities | filter_by_type: 'class' %}

{% if classes and classes.length > 0 %}
## Classes

{% for c in classes %}
- [{{ c.name }}]({{ c | get_url }})
{% endfor %}

{% endif %}

{% endif %}
