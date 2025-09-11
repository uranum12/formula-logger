import ky from "ky"

export type DataItem = {
  usec: number
  value: number
}

export type ResType = {
  [key: string]: DataItem[]
}

export async function fetchLatestData(
  fields: string[],
  limit: number = 100,
): Promise<ResType> {
  if (fields.length === 0) return {}
  return ky
    .get<ResType>(`/latest/${limit}`, {
      searchParams: { fields: fields.join(",") },
      timeout: 1000,
      retry: { limit: 0 },
    })
    .json()
}
