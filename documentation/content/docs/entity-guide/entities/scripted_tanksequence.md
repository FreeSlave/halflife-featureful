---
bookHidden: true
bookToC: false
---

# ![](/images/spirit.png) scripted_tanksequence

Allows making [func_tank]({{< ref func_tank >}}) and other types of tanks to rotate to face a certain position and fire. Use it for cinematic events when you want a tank to fire at the specified direction.

{{% hint info %}}
Unlike Spirit of Half-Life version, there's no 1 second delay before tank starts executing the sequence.
{{% /hint %}}

### Reaction to use-type

Ignores the use-type. If it already has possessed some tank, it will stop the sequence. If it doesn't have the tank, it will try to possess the tank and start the sequence.
