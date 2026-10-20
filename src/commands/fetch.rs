use anyhow::Result;

use crate::registry::get_all_records;

pub fn fetch() -> Result<()> {
    let records = get_all_records()?;

    let total = records.len();
    let git_count = records.iter().filter(|r| r.git_url.is_some()).count();

    println!("{}/{} (makekeeper)", total, git_count);

    Ok(())
}
