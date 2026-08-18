# Redesigning Resource and Command Queue Synchronization

My previous resource-tracking system could not automatically determine which command queue would use a GPU resource. I had to explicitly mark resources for graphics, compute, or copy queue usage.

For example, when blurring an image on a `D3D12_COMMAND_LIST_TYPE_COMPUTE` queue, I had to mark the image as being used by the compute engine. Later, the graphics queue had to check that marker and wait for the compute queue before rendering the blurred result.

```cpp
void blur(ImageHandle image)
{
  // Record the blur operation.
  // ...

  image.compute_engine_will_use();
}

void submit()
{
  // Submit command lists.
  // ...

  if (command.needs_compute_engine_sync())
    GraphicsEngine.wait(ComputeEngine, fence_value);
}
```

This approach required each operation to know which queues would use its resources. It also made synchronization increasingly difficult as more execution paths were added.

I redesigned the system around **resource access** rather than explicit queue ownership.

Each command now records whether it reads or writes a resource. Before submission, the tracker compares the current command's resource usages with unfinished submissions from other queues.

```cpp
void submit()
{
  // Add queue waits for conflicting resource accesses.
  wait_for_conflicting_submissions();

  // Submit the command lists.
  // ...

  // Preserve their resource usages until GPU completion.
  record_submission_usages();
}
```

The synchronization rules are:

| Previous access | Current access | Synchronization required |
|-----------------|----------------|--------------------------|
| Read            | Read           | No                       |
| Read            | Write          | Yes                      |
| Write           | Read           | Yes                      |
| Write           | Write          | Yes                      |

When a conflict is found, the current GPU queue waits on the previous queue's fence.

Resource access is recorded automatically by command APIs. In the blur path, for example, image state transitions identify source images as reads and render targets or unordered-access images as writes.

The result is a simpler model:

- Operations no longer specify which engine will use a resource.
- Synchronization is derived from actual command usage.
- Cross-queue dependencies are inserted automatically.
- Read-only work can still run concurrently across queues.

This redesign reduces manual bookkeeping and makes new graphics, compute, and copy workflows easier to implement correctly.

Commit:
https://github.com/Myc123abc/tk/commit/65248ea22f96e3ceee9f853a024db78f2de6e8ec
