<?php
namespace App\Http\Controllers\Api;
use App\Http\Controllers\Controller;
use App\Models\Device;
use App\Models\StatusHistory;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
class DeviceStatusController extends Controller {
    public function store(Request $request){
        $token=$request->bearerToken();
        if(!is_string($token)||strlen($token)!==64||!ctype_alnum($token)) return response()->json(['error'=>'Unauthenticated'],401);
        $data=$request->validate([
            'device_id'=>'required|string|max:80',
            'status'=>'required|in:available,busy,unavailable',
            'capacity'=>'sometimes|integer|min:0|max:100',
        ]);
        $device=Device::where('device_uid',$data['device_id'])->first();
        if(!$device || !hash_equals($device->token_hash,hash('sha256',$token))) return response()->json(['error'=>'Unauthenticated'],401);
        $provider=$device->provider;
        if(!$provider) return response()->json(['error'=>'Device has no provider'],422);
        DB::transaction(function()use($device,$provider,$data){
            $now=now();$newStatus=$data['status'];$capacity=$data['capacity']??($newStatus==='available'?1:0);
            $changed=$provider->status!==$newStatus || $provider->capacity!==$capacity;
            $device->update(['last_seen_at'=>$now]);
            $provider->update(['status'=>$newStatus,'capacity'=>$capacity,'last_seen_at'=>$now]);
            if($changed) StatusHistory::create(['provider_id'=>$provider->id,'device_id'=>$device->id,'status'=>$newStatus,'capacity'=>$capacity,'created_at'=>$now]);
        });
        return response()->json(['ok'=>true,'provider_id'=>$provider->id,'server_time'=>now()->toIso8601String()]);
    }
}
