# Khiops Sample Datasets

These are publicly available datasets reformatted to easily test the [Khiops AutoML
suite](https://www.khiops.com).

Each dataset directory contains:

- One or more TSV files with the data tables
- A Khiops dictionary file `.kdic` describing the table schema
- Optional supplementary information and scripts


## Datasets

### Adult

- Source: http://archive.ics.uci.edu/dataset/2/adult
- Description: Also known as "Census Income" dataset. Main task: Predict whether income exceeds
  \$50K/yr based on census data.
- Dimensions: 48842 instances, 14 variables.

### Iris
- Source: http://archive.ics.uci.edu/dataset/53/iris
- Description: The data set contains 3 classes of 50 instances each, where each class refers to
  a type of iris plant.
- Dimensions: 150 instances, 5 variables.

### Letter
- Source: https://archive.ics.uci.edu/dataset/59/letter+recognition
- Description: Database of character letter image features. Main task: Identify the letter.
- Dimensions: 20000 instances, 17 variables.

### Mushroom
- Source: http://archive.ics.uci.edu/dataset/73/mushroom
- Physical characteristics characteristics from various mushrooms, from the Audobon Society Field
  Guide. Main task: Predict if a mushroom is poisonous or edible.
- Dimension: 8124 instances, 22 variables.

### SpliceJunction
- Source: https://archive.ics.uci.edu/dataset/69/molecular+biology+splice+junction+gene+sequences
- Primate splice-junction gene sequences (DNA) with associated imperfect domain theory. Main task:
  predict the type of splice junction. Multi-table database.
- Dimensions: (after removal of duplicates among the 3190 initial instances)
   - Main entity `SpliceJunction`: 3178 instances, 2 variables.
   - Secondary entity `SpliceJunctionDNA`: 190680 instances, 2 variables.

### Accidents
-  Source: https://www.onisr.securite-routiere.gouv.fr/
-  Description: From _Observatoire national interministériel de la sécurité routière_ (ONISR),
   a traffic accident database from open data. Multi-table database.
-  Multi-table database with 4 tables in a snowflake schema, 57783 instances in the root table
- Dimensions:
  - Main entity `Accidents`: 57783 instances, 13 variables.
  - Secondary entities:
    - `Places`: 57783 instances, 18 variables.
    - `Vehicles`: 98876 instances, 9 variables.
      - `Users`: 130169 instances, 16 variables.

### AccidentsSummary
-  Simpler version of `Accidents`, containing only the `Accidents` and `Vehicles` tables and less
   columns.

### Customer
-  Tiny artificial dataset. It illustrates a snowflake schema.

### CustomerExtended
-  Tiny artificial dataset. It illustrates a snowflake schema with external tables.


Reference
=========
- Bache, K. & Lichman, M. (2013). UCI Machine Learning Repository http://archive.ics.uci.edu.
Irvine, CA: University of California, School of Information and Computer Science.
