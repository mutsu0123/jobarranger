
-- Job Arranger upgrade table SQL for PostgreSQL (Ver 6.0.8)  - 2024/08/31 -

-- Copyright (C) 2012-2014 FitechForce, Inc. All Rights Reserved.
-- Copyright (C) 2024 Daiwa Institute of Research Business Innovation Ltd. All Rights Reserved.
DROP INDEX IF EXISTS ja_run_job_idx3;

CREATE INDEX IF NOT EXISTS ja_run_job_idx3 ON ja_run_job_table (job_type, status);
CREATE INDEX IF NOT EXISTS ja_run_icon_job_idx1 ON ja_run_icon_job_table (host_flag, host_name);
CREATE UNIQUE INDEX IF NOT EXISTS ja_run_icon_job_idx2 ON ja_run_icon_job_table (inner_job_id, host_flag);
CREATE UNIQUE INDEX IF NOT EXISTS ja_run_value_before_idx2 ON ja_run_value_before_table (inner_job_id, value_name, seq_no);
CREATE UNIQUE INDEX IF NOT EXISTS ja_run_icon_reboot_idx2 ON ja_run_icon_reboot_table (inner_job_id, host_flag);
CREATE INDEX IF NOT EXISTS ja_run_flow_idx1 ON ja_run_flow_table (start_inner_job_id);

ALTER TABLE ja_run_jobnet_summary_table ADD COLUMN IF NOT EXISTS delete_flag INT DEFAULT 0 NULL;
