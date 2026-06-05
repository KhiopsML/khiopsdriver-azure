# List of tasks to complete

1. Complete implementation of the three cloud-storage drivers (GCS, S3, Azure):
   - (a) Share user parameter validation functions among drivers, based on the GCS driver implementation. Update related tests for S3 and Azure drivers.
   - (b) Implement function `driver_composeMultifile` for S3 and Azure drivers based on the GCS driver implementation. Update tests.
   - (c) Update function `driver_remove` for S3 and Azure drivers based on the GCS driver implementation. Update tests.
   - (d) Define specification of function `driver_rmdir` to allow recursive removal and/or globbing-based removal. Implement functionality and tests for S3, GCS and Azure drivers.
2. Complete packaging of GCS, S3 and Azure drivers:
   - (a) Complete pip packaging.
   - (b) Update conda, DEB and RPM packages to the latest driver versions.
3. Evaluate performance of GCS, S3 and Azure drivers, in the context of Khiops execution:
   - (a) Setup test environments (project, VM/notebook, Khiops and driver installation) for multiple CPU configurations using scripts or Terraform.
   - (b) Implement test scenarii: learning, sort&deploy, for multiple buffer sizes (8 MiB, 16 MiB, 32 MiB, 64 MiB).
   - (c) Execute tests and write measurements into a report.
   - (d) *(optional)* Identify bottlenecks and reduce their impact on performance. Implement additional test scenarii.