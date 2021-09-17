---
title: Home
description: Home page
---

# Endless adventure starts here

![Five children on their electric cars](/five-kids-and-cars.jpg){:class="img-responsive"}
<br/><br/>

# They put smiles on faces
![Girl with big smile on electric car](/sophie-birthday.jpg){:class="img-responsive"}
<br/><br/>

# Tested by kids
![Electric car crashed in drain](/ethan-crash-drain.jpg){:class="img-responsive"}
<br/><br/>

# Reliable and robust
![Electric car carrying many bricks](/ethan-bricks.jpg){:class="img-responsive"}

<ul>
  {% for item in include.nav %}
    <li><a href="{{ item.url }}">{{ item.title }}</a></li>

    {% if item.subnav %}
      {% include nav.html nav=item.subnav %}
    {% endif %}
  {% endfor %}
</ul>
