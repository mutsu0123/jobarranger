
-- Job Arranger delete garbage jobnets data SQL for PostgreSQL (Ver 6.0.8)  - 2024/10/02 -

-- Copyright (C) 2012-2014 FitechForce, Inc. All Rights Reserved.
-- Copyright (C) 2024 Daiwa Institute of Research Business Innovation Ltd. All Rights Reserved.

-- change YOUR_DELETE_DATE
-- eg format: '2024082200000000'
DELETE FROM ja_run_jobnet_table
WHERE inner_jobnet_id IN ( SELECT inner_jobnet_id
    FROM ja_run_jobnet_table
    WHERE status in (3, 5) AND inner_jobnet_main_id NOT IN (
            SELECT inner_jobnet_id FROM ja_run_jobnet_summary_table WHERE status NOT IN (3, 5)
    ) AND end_time <= YOUR_DELETE_DATE LIMIT 100
);
