{% if markdown and markdown.just_the_docs %}
---
layout: default
title: "{{ class.name }} class"
parent: "Class reference"
---
{% endif %}

# {{ class.name }} Class

{% if class.brief %}
**Brief:** {{ class.brief }}
{% endif %}

## Detailed description

{{ class.description }}

{% assign member_enums = class.members | filter_by_type: 'enum' %}

{% if member_enums.length > 0 %}
## Enumerations
{% for enm in member_enums %}
{% include enum_in_class with enum = enm %}
{% endfor %}
{% endif %}

## Members documentation

{% for f in class.members | filter_by_type: 'function' %}

### {{ f | funsig | markdown_escape }}

{% if f.brief %}
**Brief:** {{ f.brief }}
{% endif %}

{% if f | has_any_documented_param %}
Parameters:
{% for p in f.parameters %}
- {{ p | param_brief_or_name }}
{% endfor %}
{% endif %}

{% if f.returns %}
**Returns:** {{ f.returns }}
{% endif %}

{{ f.description }}

{% endfor %}

{% assign non_members = class | related_non_members %}
{% if non_members.length > 0 %}
## Non-Members documentation

{% for f in non_members %}

### {{ f | funsig | markdown_escape }}

{% if f.brief %}
**Brief:** {{ f.brief }}
{% endif %}

{% if f | has_any_documented_param %}
Parameters:
{% for p in f.parameters %}
- {{ p | param_brief_or_name }}{% newline %}
{% endfor %}
{% endif %}

{% if f.returns %}
**Returns:** {{ f.returns }}
{% endif %}

{{ f.description }}

{% endfor %}

{% endif %}